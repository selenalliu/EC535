#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h> /* filesystem operations */
#include <linux/timer.h> /* timer functionality */
#include <linux/uaccess.h> /* copy_to/from_user */

#define MAX_TIMERS 5

MODULE_LICENSE("Dual BSD/GPL");

/* Major number */
static int mytimer_major = 61;
static int max_active_timers = 1; // default, change with -m flag
static char message_type = '\0'; // U for updated timer, R for reached max timers, \0 for neither
static bool list_timers = false; // flag for listing timers
static int update_timer_index; // index of timer to update

typedef struct {
	struct timer_list timer;
	int seconds;
	char message[128];
	unsigned long start_time; // in jiffies, for calculating expiration time
	bool active;
} mytimer_t;

static mytimer_t timers[MAX_TIMERS];

static int mytimer_open(struct inode *inode, struct file *filp);
static int mytimer_release(struct inode *inode, struct file *filp);
static ssize_t mytimer_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t mytimer_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static int mytimer_init(void);
static void mytimer_exit(void);

/* Structure for file access operations */
struct file_operations mytimer_fops = {
	open : mytimer_open,
	release : mytimer_release,
	read : mytimer_read,
	write : mytimer_write
};

module_init(mytimer_init);
module_exit(mytimer_exit);

static void mytimer_callback(struct timer_list *t)
{
    mytimer_t *mytimer = from_timer(mytimer, t, timer);
    printk(KERN_INFO "%s\n", mytimer->message);
    mytimer->active = false;
}

static int mytimer_init(void) {
	// register device file
	int result;
	result = register_chrdev(mytimer_major, "mytimer", &mytimer_fops);
	if (result < 0) {
		// printk(KERN_ALERT "mytimer: cannot obtain major number %d\n", mytimer_major);
		return result;
	}

	// init timers
	memset(timers, 0, sizeof(timers));

	// printk(KERN_ALERT "Inserting mytimer module\n");
	return 0;
}

static void mytimer_exit(void) {
	int i;
	// unregister device
	unregister_chrdev(mytimer_major, "mytimer");

	// free timers
	for (i = 0; i < MAX_TIMERS; i++) {
		if (timers[i].seconds > 0) {
			del_timer(&timers[i].timer);
		}
	}

	// printk(KERN_ALERT "Removing mytimer module\n");
}

static int mytimer_open(struct inode *inode, struct file *filp) {
	// printk(KERN_INFO "open called: process id %d, command %s\n",
	// 		current->pid, current->comm);
	// Success
	return 0;
}

static int mytimer_release(struct inode *inode, struct file *filp) {
	// printk(KERN_INFO "release called: process id %d, command %s\n",
	// 		current->pid, current->comm);
	// Success
	return 0;
}

// handle all file reads from userspace
static ssize_t mytimer_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
	int i;
	char tbuf[256], *tbptr = tbuf; // temporary buffer + pointer
	size_t len;

	if (*f_pos > 0) {
        return 0; // EOF
    }

	/*************************************************************/

	// for -s print messages
	if (!list_timers) {	
		if (message_type == 'U') {
			tbptr += sprintf(tbptr, "The timer %s was updated", timers[update_timer_index].message);
		} else if (message_type == 'R') {
			tbptr += sprintf(tbptr, "%d timer(s) already exist(s)!", max_active_timers);
		} else {
			return 0;
		}

		len = tbptr - tbuf;
		// limit count to prevent buffer overflows
		if (count > len - *f_pos) {
			count = len - *f_pos;
		}
		if (copy_to_user(buf, tbuf + *f_pos, count)) {
			return -EFAULT;
		}
		*f_pos += count;
		message_type = '\0';
		return count;
	}

	
	/*************************************************************/

	// list active timers to stdout (-l)
	for (i = 0; i < max_active_timers; i++) {
		if (timers[i].active) {
			// calculate seconds remaining
			unsigned long elapsed_jiffies = jiffies - timers[i].start_time;
            unsigned long elapsed_seconds = elapsed_jiffies / HZ;
            int expiration_time = timers[i].seconds - elapsed_seconds;

            if (expiration_time < 0) {
				expiration_time = 0; // ensure non-negative
			}

			// print timer info (seconds remaining & message)
			tbptr += sprintf(tbptr, "%s %d\n", timers[i].message, expiration_time);
		}
	}

	len = tbptr - tbuf; // length of string in temporary buffer

	// if no timers pending (nothing in buffer), print nothing
	if (len == 0) {
		return 0;
	}

	// limit count to prevent buffer overflows
	if (count > len - *f_pos) {
		count = len - *f_pos;
	}

	// copy to user, check for errors
	if (copy_to_user(buf, tbuf + *f_pos, count)) {
		return -EFAULT;
	}

	*f_pos += count; // increment file position
	list_timers = false; // reset flag
	return count;
}

// handle all file writes from userspace
static ssize_t mytimer_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
	char kbuf[256];
	int i, sec, new_max;
	char msg[128];

	if (count > sizeof(kbuf) - 1) {
		return -EINVAL;
	}

	// copy from user, check for errors
	if (copy_from_user(kbuf, buf, count)) {
		return -EFAULT;
	}

	kbuf[count] = '\0'; // null terminate string

	// check for set timer (-s)
	if (sscanf(kbuf, "s %d %127[^\n]", &sec, msg) == 2) {
		// if active timer with same message exists
			// reset timer's remaining time to [SEC]
			// print "The timer <MSG> was updated!" 
		for (i = 0; i < max_active_timers; i++) {
			if (timers[i].active && strcmp(timers[i].message, msg) == 0) {
				// update timer
				timers[i].start_time = jiffies; // update start time to current time (in jiffies)
				mod_timer(&timers[i].timer, jiffies + sec * HZ);
				// reset timer's remaining time to [SEC]
				timers[i].seconds = sec;

				// set message type to updated
				message_type = 'U';
				update_timer_index = i; // update with current timer index
				return count;
				
			} else if (!timers[i].active) {
				strcpy(timers[i].message, msg);
                timers[i].seconds = sec;
				timers[i].start_time = jiffies; // set start time to current time (in jiffies)
                timers[i].active = true;
                timer_setup(&timers[i].timer, mytimer_callback, 0);
                mod_timer(&timers[i].timer, jiffies + sec * HZ);
				message_type = '\0';
				return count;
			}
		}

		// if adding new msg timer over kernel module limit
			// print "[COUNT] timer(s) already exist(s)!"
		if (i == max_active_timers) {
			// set message type to reached max timers
			message_type = 'R';
			return count;
		}

	} else if (sscanf(kbuf, "m %d", &new_max) == 1) { // set max active timer count (-m)
		// change max active timer count to given [COUNT]
		// check for valid input
		if (new_max < 1 || new_max > MAX_TIMERS) {
			// printk(KERN_INFO "Invalid max timer count!\n");
			return -EINVAL;
		} else {
			max_active_timers = new_max;
			return count;
		}
		
	} else if (sscanf(kbuf, "l") == 0) { // list active timers (-l)
		list_timers = true;
		return count;
	}
	return -EINVAL;
}