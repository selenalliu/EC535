
/* Necessary includes for device drivers */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/jiffies.h> /* jiffies */
#include <asm/system_misc.h> /* cli(), *_flags */
#include <asm/uaccess.h> /* copy_from/to_user */
#include <linux/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");

static int fasync_example_fasync(int fd, struct file *filp, int mode);
static int fasync_example_open(struct inode *inode, struct file *filp);
static int fasync_example_release(struct inode *inode, struct file *filp);
static ssize_t fasync_example_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static int fasync_example_fasync(int fd, struct file *filp, int mode);
static int fasync_example_init(void);
static void fasync_example_exit(void);
static void timer_handler(struct timer_list*);


/*
 * The file operations for the pipe device
 * (some are overlayed with bare scull)
 */
struct file_operations fasync_example_fops = {
write:
    fasync_example_write,
open:
    fasync_example_open,
release:
    fasync_example_release,
fasync:
    fasync_example_fasync
};

/* Declaration of the init and exit functions */
module_init(fasync_example_init);
module_exit(fasync_example_exit);

static int fasync_example_major = 61; /* be sure to run mknod with this major num! */
struct fasync_struct *async_queue; /* structure for keeping track of asynchronous readers */
static struct timer_list * fasync_timer; /* structure for keeping track of timer */

static int fasync_example_init(void) {
    int result;

    /* Registering device */
    result = register_chrdev(fasync_example_major, "fasync_example", &fasync_example_fops);
    if (result < 0)
    {
        printk(KERN_ALERT
               "fasync_example: cannot obtain major number %d\n", fasync_example_major);
        return result;
    }

    /* Allocating buffers */
    fasync_timer = (struct timer_list *) kmalloc(sizeof(struct timer_list), GFP_KERNEL);

    /* Check if timer  */
    if (!fasync_timer)
    {
        printk(KERN_ALERT "Insufficient kernel memory\n");
        result = -ENOMEM;
        goto fail;
    }

    printk("fasync_example loaded.\n");
    return 0;

fail:
    fasync_example_exit();
    return result;
}

static void fasync_example_exit(void) {
    /* Freeing the major number */
    unregister_chrdev(fasync_example_major, "fasync_example");
    if (fasync_timer)
        kfree(fasync_timer);

    printk(KERN_ALERT "Removing fasync_example module\n");

}

static int fasync_example_open(struct inode *inode, struct file *filp) {
    return 0;
}

static int fasync_example_release(struct inode *inode, struct file *filp) {
    fasync_example_fasync(-1, filp, 0);
    return 0;
}

static ssize_t fasync_example_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    timer_setup(fasync_timer, timer_handler, 0);
    mod_timer(fasync_timer, jiffies + msecs_to_jiffies(10000));
    return count;
}

static int fasync_example_fasync(int fd, struct file *filp, int mode) {
    return fasync_helper(fd, filp, mode, &async_queue);
}

static void timer_handler(struct timer_list *data) {
    if (async_queue)
        kill_fasync(&async_queue, SIGIO, POLL_IN);

    del_timer(fasync_timer);
}

