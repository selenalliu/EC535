/* Code from
http://www.freesoftwaremagazine.com/articles/drivers_linux?page=0%2C3
*/

/* Necessary includes for device drivers */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h>   /* kmalloc() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>     /* O_ACCMODE */
#include <asm/system_misc.h> /* cli(), *_flags */
#include <linux/uaccess.h>
#include <asm/uaccess.h> /* copy_from/to_user */

MODULE_LICENSE("Dual BSD/GPL");

/* Declaration of memory.c functions */
static int memory_open(struct inode *inode, struct file *filp);
static int memory_release(struct inode *inode, struct file *filp);
static ssize_t memory_read(struct file *filp,
                           char *buf, size_t count, loff_t *f_pos);
static ssize_t memory_write(struct file *filp,
                            const char *buf, size_t count, loff_t *f_pos);
static void memory_exit(void);
static int memory_init(void);

/* Structure that declares the usual file */
/* access functions */
struct file_operations memory_fops =
    {
            read : memory_read,
            write : memory_write,
            open : memory_open,
            release : memory_release
    };

/* Declaration of the init and exit functions */
module_init(memory_init);
module_exit(memory_exit);

/* Global variables of the driver */
/* Major number */
static int memory_major = 60;
/* Buffer to store data */
static char *memory_buffer;

static int memory_init(void)
{
        int result;

        /* Registering device */
        result = register_chrdev(memory_major, "memory", &memory_fops);
        if (result < 0)
        {
                printk(KERN_ALERT
                       "memory: cannot obtain major number %d\n",
                       memory_major);
                return result;
        }

        /* Allocating memory for the buffer */
        memory_buffer = kmalloc(1, GFP_KERNEL);
        if (!memory_buffer)
        {
                printk(KERN_ALERT "Insufficient kernel memory\n");
                result = -ENOMEM;
                goto fail;
        }
        memset(memory_buffer, 0, 1);

        printk(KERN_ALERT "Inserting memory module\n");
        return 0;

fail:
        memory_exit();
        return result;
}

static void memory_exit(void)
{
        /* Freeing the major number */
        unregister_chrdev(memory_major, "memory");

        /* Freeing buffer memory */
        if (memory_buffer)
        {
                kfree(memory_buffer);
        }

        printk(KERN_ALERT "Removing memory module\n");
}

static int memory_open(struct inode *inode, struct file *filp)
{
        printk(KERN_INFO "open called: process id %d, command %s\n",
               current->pid, current->comm);
        /* Success */
        return 0;
}

static int memory_release(struct inode *inode, struct file *filp)
{
        printk(KERN_INFO "release called: process id %d, command %s\n",
               current->pid, current->comm);
        /* Success */
        return 0;
}

static ssize_t memory_read(struct file *filp, char *buf,
                           size_t count, loff_t *f_pos)
{
        /* Transfering data to user space */
        if (copy_to_user(buf, memory_buffer, 1))
        {
                return -EFAULT;
        }

        printk(KERN_INFO "read called: process id %d, command %s, char %d\n",
               current->pid, current->comm, *memory_buffer);

        /* Changing reading position as best suits */
        if (*f_pos == 0)
        {
                *f_pos += 1;
                return 1;
        }
        else
        {
                return 0;
        }
}
static ssize_t memory_write(struct file *filp, const char *buf,
                            size_t count, loff_t *f_pos)
{
        const char *tmp;

        tmp = buf + count - 1;
        if (copy_from_user(memory_buffer, tmp, 1))
        {
                return -EFAULT;
        }

        printk(KERN_INFO "write called: process id %d, command %s, char %d\n",
               current->pid, current->comm, *memory_buffer);
        return 1;
}