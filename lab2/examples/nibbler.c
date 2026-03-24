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
static int nibbler_open(struct inode *inode, struct file *filp);
static int nibbler_release(struct inode *inode, struct file *filp);
static ssize_t nibbler_read(struct file *filp,
                            char *buf, size_t count, loff_t *f_pos);
static ssize_t nibbler_write(struct file *filp,
                             const char *buf, size_t count, loff_t *f_pos);
static void nibbler_exit(void);
static int nibbler_init(void);

/* Structure that declares the usual file */
/* access functions */
struct file_operations nibbler_fops = {
        read : nibbler_read,
        write : nibbler_write,
        open : nibbler_open,
        release : nibbler_release
};

/* Declaration of the init and exit functions */
module_init(nibbler_init);
module_exit(nibbler_exit);

static unsigned capacity = 128;
static unsigned bite = 128;
module_param(capacity, uint, S_IRUGO);
module_param(bite, uint, S_IRUGO);

/* Global variables of the driver */
/* Major number */
static int nibbler_major = 61;

/* Buffer to store data */ 
static char *nibbler_buffer; /* length of the current message */
static int nibbler_len;
static int nibbler_init(void)
{
        int result; /* Registering device */
        result = register_chrdev(nibbler_major, "nibbler", &nibbler_fops);
        if (result < 0)
        {
                printk(KERN_ALERT "nibbler: cannot obtain major number %d\n", nibbler_major);
                return result;
        }

        /* Allocating nibbler for the buffer */
        nibbler_buffer = kmalloc(capacity, GFP_KERNEL);
        if (!nibbler_buffer)
        {
                printk(KERN_ALERT "Insufficient kernel memory\n");
                result = -ENOMEM;
                goto fail;
        }
        memset(nibbler_buffer, 0, capacity);
        nibbler_len = 0;

        printk(KERN_ALERT "Inserting nibbler module\n");
        return 0;

fail:
        nibbler_exit();
        return result;
}

static void nibbler_exit(void)
{
        /* Freeing the major number */
        unregister_chrdev(nibbler_major, "nibbler");

        /* Freeing buffer memory */
        if (nibbler_buffer)
        {
                kfree(nibbler_buffer);
        }

        printk(KERN_ALERT "Removing nibbler module\n");
}

static int nibbler_open(struct inode *inode, struct file *filp)
{
        printk(KERN_INFO "open called: process id %d, command %s\n",
               current->pid, current->comm);
        /* Success */
        return 0;
}

static int nibbler_release(struct inode *inode, struct file *filp)
{
        printk(KERN_INFO "release called: process id %d, command %s\n",
               current->pid, current->comm);
        /* Success */
        return 0;
}

static ssize_t nibbler_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
        int temp;
        char tbuf[256], *tbptr = tbuf;

        /* end of buffer reached */
        if (*f_pos >= nibbler_len)
        {
                return 0;
        }

        /* do not go over then end */
        if (count > nibbler_len - *f_pos)
                count = nibbler_len - *f_pos;

        /* do not send back more than a bite */
        if (count > bite)
                count = bite;

        /* Transfering data to user space */
        if (copy_to_user(buf, nibbler_buffer + *f_pos, count))
        {
                return -EFAULT;
        }

        tbptr += sprintf(tbptr,

                         "read called: process id %d, command %s, count %d, chars ",
                         current->pid, current->comm, count);

        for (temp = *f_pos; temp < count + *f_pos; temp++)

                tbptr += sprintf(tbptr, "%c", nibbler_buffer[temp]);

        printk(KERN_INFO "%s\n", tbuf);

        /* Changing reading position as best suits */
        *f_pos += count;
        return count;
}

static ssize_t nibbler_write(struct file *filp, const char *buf,
                             size_t count, loff_t *f_pos)
{
        int temp;
        char tbuf[256], *tbptr = tbuf;

        /* end of buffer reached */
        if (*f_pos >= capacity)
        {
                printk(KERN_INFO
                       "write called: process id %d, command %s, count %d, buffer full\n",
                       current->pid, current->comm, count);
                return -ENOSPC;
        }

        /* do not eat more than a bite */
        if (count > bite)
                count = bite;

        /* do not go over the end */
        if (count > capacity - *f_pos)
                count = capacity - *f_pos;

        if (copy_from_user(nibbler_buffer + *f_pos, buf, count))
        {
                return -EFAULT;
        }

        tbptr += sprintf(tbptr,

                         "write called: process id %d, command %s, count %d, chars ", current->pid, current->comm, count);
        for (temp = *f_pos; temp < count + *f_pos; temp++)

                tbptr += sprintf(tbptr, "%c", nibbler_buffer[temp]);

        printk(KERN_INFO "%s\n", tbuf);

        *f_pos += count;
        nibbler_len = *f_pos;

        return count;
}