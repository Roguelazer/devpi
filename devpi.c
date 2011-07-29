/**
 * /dev/pi: irrationally better
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/random.h>
#include <asm/uaccess.h>
#include <asm/segment.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include "devpi.h"

#define MIN(a,b) ((a)>(b)?(b):(a))
#define WRITE_SIZE 4096

/*****************************
 *
 * module preamble
 *
 ****************************/
MODULE_AUTHOR("James Brown <jbrown@yelp.com>");
MODULE_AUTHOR("Evan Klitzke <evan@yelp.com>");
MODULE_LICENSE("GPL");

/******************************
 *
 * sysctl interface
 *
 *****************************/
#ifdef CONFIG_SYSCTL
#include <linux/sysctl.h>
static enum pi_mode mode = DECIMAL;
static unsigned max_opened = 32;
static size_t max_decimal_size = (1<<20);
static unsigned min_opened_threshold = 1;
static unsigned max_opened_threshold = 2;

/* Read the string equivalents of the enum pi_mode modes */
static int proc_dopimode(ctl_table* table, int write, void __user* buffer, size_t* lenp, loff_t* ppos)
{
    ctl_table fake_table;
    char buf[10];
    enum pi_mode pmode;

    if (!write) {
        pmode = *((enum pi_mode*)table->data);
        if ((pmode < PI_MODE_MIN) || (pmode > PI_MODE_MAX)) {
            return -EINVAL;
        }
        sprintf(buf, "%s", pi_mode_map[pmode]);
        fake_table.data = buf;
        fake_table.maxlen = sizeof(buf);
        return proc_dostring(&fake_table, write, buffer, lenp, ppos);
    } else {
        size_t bytes_to_write = MIN(10, *lenp);
        int bytes = copy_from_user(buf, buffer, bytes_to_write);
        if (bytes != 0) {
            return -EINVAL;
        }
        if ((bytes_to_write >= 7) && (strncmp(buf, "decimal", 7) == 0)) {
            mode = DECIMAL;
        } else if ((bytes_to_write >= 3) && strncmp(buf, "hex", 3) == 0) {
            mode = HEX;
        } else {
            return -EINVAL;
        }
        return 0;
    }
}

static ctl_table pi_table[] = {
    {
        .procname = "max_opened",
        .data = &max_opened,
        .maxlen = sizeof(unsigned),
        .mode = 0644,
        .proc_handler = proc_dointvec_minmax,
        .extra1 = &min_opened_threshold,
        .extra2 = &max_opened_threshold,
    },
    {
        .procname = "max_decimal_size",
        .data = &max_decimal_size,
        .maxlen = sizeof(size_t),
        .mode = 0644,
        .proc_handler = proc_dointvec,
    },
    {
        .procname = "mode",
        .data = &mode,
        .proc_handler = proc_dopimode,
        .mode = 0644,
    },
    { }
};
static ctl_table pi_root[] = {
    {
        .procname = "pi",
        .mode = 0555,
        .child = pi_table,
    },
    { }
};
static ctl_table dev_root[] = {
    {
        .procname = "dev",
        .mode = 0555,
        .child = pi_root,
    },
    { }
};
static struct ctl_table_header* sysctl_handler;
static int __init init_sysctl(void)
{
    sysctl_handler = register_sysctl_table(dev_root);
    return 0;
}

static int __exit cleanup_sysctl(void)
{
    unregister_sysctl_table(sysctl_handler);
    return 0;
}
#else
#define max_opened 32
#define max_decimal_size (1<<20)
#define current_mode DECIMAL
#endif /* CONFIG_SYSCTL */

/******************************
 *
 * Implementation predeclarations
 * (some of these are in devpi.h)
 *
 *****************************/
static int device_open(struct inode*, struct file*);
static int device_release(struct inode*, struct file*);
static ssize_t device_read(struct file*, char __user *, size_t, loff_t);

static int opened;

static struct class* pi_class;

static struct file_operations fops = {
    .read = device_read,
    .open = device_open,
    .release = device_release
};

/******************************
 *
 * Actual implementation
 *
 *****************************/

static int device_open(struct inode* inodp, struct file* filp)
{
    if (opened > max_opened)
        return -EBUSY;
    opened++;
    try_module_get(THIS_MODULE);
    filp->private_data = kmalloc(sizeof(struct pi_status), GFP_KERNEL);
    if (filp->private_data == NULL)
        return -ENOMEM;
    ((struct pi_status*)filp->private_data)->bytes_read = 0;
    return 0;
}

static int device_release(struct inode* inodp, struct file* filp)
{
    opened--;
    if (filp->private_data)
        kfree(filp->private_data);
    module_put(THIS_MODULE);
    return 0;
}

static ssize_t device_read(struct file* filp, char __user * buffer, size_t length, loff_t offset)
{
    struct pi_status* status = (struct pi_status*)filp->private_data;
    size_t total_bytes_read = status->bytes_read;
    size_t total_length = total_bytes_read + length;
    size_t bytes_read = 0;

    int size = (total_length >> 2) * 14;
    int* r;
    char* orig_cbuf;
    char* cptr;
    int i, k;
    int b, d;
    int c = 0;
    if (size >= max_decimal_size) {
        return -E2BIG;
    }
    r = kmalloc(size * sizeof(int) + 1, GFP_USER);
    orig_cbuf = kmalloc((length + 2) * sizeof(char), GFP_USER);
    cptr = orig_cbuf;
    printk(KERN_INFO "Allocated orig_cbuf=%p, r=%p\n", orig_cbuf, r);
    if (r == NULL) {
        printk(KERN_INFO "r allocation failed\n");
        return -ENOMEM;
    } else if (orig_cbuf == NULL) {
        printk(KERN_INFO "cbuf allocation failed\n");
        kfree(r);
        return -ENOMEM;
    }
    // Compute pi
    printk(KERN_INFO "Beginning PI computation for %zd digits, creating %zd bytes for r and %zd bytes for cbuf\n", total_length, size * sizeof(int) + 1, (length + 1) * sizeof(char));
    for (i = 0; i < size; ++i) {
        //r[i] = size - total_length;
        r[i] = 2000;
    }
    for (k = size; k > 0; k -= 14) {
        d = 0;
        i = k;
        while (1) {
            d += r[i] * 10000;
            b = 2 * i - 1;
            r[i] = d % b;
            d /= b;
            i--;
            if (i == 0)
                break;
            d *= i;
        }
        printk(KERN_INFO "Computed digits %.4d", c+d/10000);
        // sprintf tacks on an extra NUL byte, which is annoying.
        // just make the cbuf one byte larger because I don't want to
        // reimplement sprintf
        sprintf(cptr, "%.4d", c+d/10000);
        cptr += 4;
        c = d % 10000;
    }
    cptr = orig_cbuf;
    printk(KERN_INFO "Copying to userspace\n");
    while (bytes_read < length) {
        size_t unwritten;
        size_t written;
        size_t bytes_to_write = MIN(length - bytes_read, WRITE_SIZE);
        printk(KERN_INFO "Writing %zd bytes\n", bytes_to_write);
        unwritten = copy_to_user(buffer, cptr, bytes_to_write);
        written = bytes_to_write - written;
        printk(KERN_INFO "Wrote %zd bytes\n", written);
        cptr += written;
        buffer += written;
        bytes_read += written;
    }
    if (orig_cbuf) {
        printk(KERN_INFO "Freeing orig_cbuf %p\n", orig_cbuf);
        kfree(orig_cbuf);
        orig_cbuf = NULL;
    }
    if (r) {
        printk(KERN_INFO "Freeing r %p\n", r);
        kfree(r);
        r = NULL;
    }
    status->bytes_read += bytes_read;
    return bytes_read;
}

int __init init_module(void)
{
    int err;

    err = register_chrdev(PI_MAJOR, PI_DEVNAME, &fops);
    if (err < 0)
        printk(KERN_ALERT "Registering char device failed with %d\n", err);

    pi_class = class_create(THIS_MODULE, PI_CLASSNAME);

    if (IS_ERR(pi_class))
        return PTR_ERR(pi_class);

    device_create(pi_class, NULL, MKDEV(PI_MAJOR, 1), NULL, PI_DEVNAME);
#ifdef CONFIG_SYSCTL
    (void) init_sysctl();
#endif /* CONFIG_SYSCTL */
    return 0;
}

void __exit cleanup_module(void)
{
    device_destroy(pi_class, MKDEV(PI_MAJOR, 1));
    unregister_chrdev(PI_MAJOR, PI_DEVNAME);
    class_destroy(pi_class);
#ifdef CONFIG_SYSCTL
    (void) cleanup_sysctl();
#endif /* CONFIG_SYSCTL */
}

