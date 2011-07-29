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
#define BUF_SIZE 32
#define MAX_OPENED 32

MODULE_AUTHOR("James Brown <jbrown@yelp.com>");
MODULE_AUTHOR("Evan Klitzke <evan@yelp.com>");
MODULE_LICENSE("GPL");

/* Implementation predeclarations */
static int device_open(struct inode*, struct file*);
static int device_release(struct inode*, struct file*);
static ssize_t device_read(struct file*, char __user *, size_t, loff_t);

static int opened;

static const char* msg = "3.14159";
ssize_t msg_len;
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
    ((struct pi_status*)filp->private_data)->bytes_read = 0;
    if (filp->private_data == NULL)
        return -ENOMEM;
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
    int* r = kmalloc(size * sizeof(int) + 1, GFP_KERNEL); 
    char* cbuf = kmalloc((length + 1) * sizeof(char), GFP_KERNEL);
    char* cptr;
    int i, k;
    int b, d;
    int c = 0;
    // Compute pi
    printk(KERN_INFO "Beginning PI computation for %zd digits, creating %zd bytes for r and %zd bytes for cbuf\n", total_length, size * sizeof(int) + 1, (length + 1) * sizeof(char));
    for (i = 0; i < size; ++i) {
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
        sprintf(cbuf, "%.4d", c+d/10000);
        cbuf += 4;
        c = d % 10000;
    }
    printk(KERN_INFO "Copying to userspace\n");
    while (length) {
        size_t unwritten;
        size_t bytes_to_write = length;
        unwritten = copy_to_user(buffer, cptr, bytes_to_write);
        cptr += (bytes_to_write - unwritten);
        buffer += (bytes_to_write - unwritten);
        bytes_read += (bytes_to_write - unwritten);
        length -= (bytes_to_write - unwritten);
    }
    kfree(cbuf);
    kfree(r);
    status->bytes_read += bytes_read;
    return bytes_read;
}

int init_module(void)
{
    int err;
    msg_len = strlen(msg);
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

