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

#include "devpi.h"

#define MIN(a,b) ((a)>(b)?(b):(a))
#define PI_MAJOR 235
#define BUF_SIZE 32

MODULE_AUTHOR("James Brown <jbrown@yelp.com>");
MODULE_AUTHOR("Evan Klitzke <evan@yelp.com>");
MODULE_LICENSE("GPL");

/* Implementation predeclarations */
static int device_open(struct inode*, struct file*);
static int device_release(struct inode*, struct file*);
static ssize_t device_read(struct file*, char __user *, size_t, loff_t);

static int opened;

static const char* msg = "3.14159";
static struct class* pi_class;

static struct file_operations fops = {
    .read = device_read,
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

static ssize_t device_read(struct file* filp, char __user * buffer, size_t length, loff_t offset)
{
    int bytes_read;
    char* msgPtr = (char*) msg;
    while (length && *msgPtr) {
        put_user(*(msgPtr++), buffer++);
        length--;
        bytes_read++;
    }
    while (length) {
        unsigned char rndbuf[BUF_SIZE];
        size_t unwritten;
        size_t bytes_to_write = MIN(BUF_SIZE, length);
        int i;
        get_random_bytes(rndbuf, BUF_SIZE);
        for (i = 0; i < bytes_to_write; ++i) {
            char new_value = (rndbuf[i] % 10);
            rndbuf[i] = new_value + 48;
        }
        rndbuf[bytes_to_write + 1] = '\0';
        unwritten = copy_to_user(buffer, rndbuf, bytes_to_write-1);
        buffer += (bytes_to_write - unwritten);
        bytes_read += (bytes_to_write - unwritten);
        length -= (bytes_to_write - unwritten);
    }
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

