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
#define PI_MAJOR 235
#define WRITE_SIZE 4096
#define MAX_OPENED 32
#define MAX_SIZE (1<<20)

MODULE_AUTHOR("James Brown <jbrown@yelp.com>");
MODULE_AUTHOR("Evan Klitzke <evan@yelp.com>");
MODULE_LICENSE("GPL");

/* Implementation predeclarations */
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

static int device_open(struct inode* inodp, struct file* filp)
{
    if (opened > MAX_OPENED)
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
    if (size >= MAX_SIZE) {
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

int init_module(void)
{
    int err;
    printk(KERN_INFO "Entering init_module");
    err = register_chrdev(PI_MAJOR, DEVICE_NAME, &fops);

    if (err < 0)
        printk(KERN_ALERT "Registering char device failed with %d\n", err);

    pi_class = class_create(THIS_MODULE, "pi");

    if (IS_ERR(pi_class))
        return PTR_ERR(pi_class);

    device_create(pi_class, NULL, MKDEV(PI_MAJOR, 1), NULL, "pi");
    return 0;
}

void cleanup_module(void)
{
    printk(KERN_INFO "Entering cleanup_module");
    device_destroy(pi_class, MKDEV(PI_MAJOR, 1));
    unregister_chrdev(PI_MAJOR, DEVICE_NAME);
    class_destroy(pi_class);
}

