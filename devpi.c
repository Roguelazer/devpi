/**
 * /dev/pi: irrationally better
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

#include "devpi.h"

#define PI_MAJOR 235

MODULE_AUTHOR("James Brown <jbrown@yelp.com>");
MODULE_AUTHOR("Evan Klitzke <evan@yelp.com>");
MODULE_LICENSE("GPL");

/* Implementation predeclarations */
static int device_open(struct inode*, struct file*);
static int device_release(struct inode*, struct file*);
static ssize_t device_read(struct file*, char* __user, size_t, loff_t);
static ssize_t device_write(struct file*, const char*, size_t, loff_t*);

static int Major;
static int opened;
static const char* msg = "3.141592";

static struct class* pi_class;

static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};

static int device_open(struct inode* inode, struct file* file)
{
    if (opened)
        return -EBUSY;
    opened++;
    try_module_get(THIS_MODULE);
    return 0;
}

static int device_release(struct inode* inode, struct file* file)
{
    opened--;
    module_put(THIS_MODULE);
    return 0;
}

static ssize_t device_read(struct file* filp, char* __user buffer, size_t length, loff_t offset)
{
    int bytes_read;
    char* msgPtr = (char*) msg;
    while (length && *msgPtr) {
        put_user(*(msgPtr++), buffer++);
        length--;
        bytes_read++;
    }
    return bytes_read;
}

static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off)
{
    printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
    return -EINVAL;
}

static char *pi_devnode(struct device *dev, mode_t *mode)
{
    if (mode && 0666) {
        *mode = 0666;
    }
    return NULL;
}

int init_module(void)
{
    printk(KERN_INFO "Entering init_module");
    Major = register_chrdev(PI_MAJOR, DEVICE_NAME, &fops);

    if (Major < 0) {
        printk(KERN_ALERT "Registering char device failed with %d\n", Major);
        return Major;
    }
    pi_class = class_create(THIS_MODULE, "pi");

    if (IS_ERR(pi_class))
        return PTR_ERR(pi_class);

    pi_class->devnode = pi_devnode;
    device_create(pi_class, NULL, MKDEV(PI_MAJOR, 1), NULL, "pi");
    return 0;
}

void cleanup_module(void)
{
    device_destroy(pi_class, MKDEV(PI_MAJOR, 1));
    unregister_chrdev(Major, DEVICE_NAME);
    class_destroy(pi_class);
}

