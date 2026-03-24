#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* filesystem operations */
#include <linux/timer.h> /* timer functionality */
#include <linux/uaccess.h> /* copy_to/from_user */
#include <linux/sched/signal.h> 
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

MODULE_LICENSE("Dual BSD/GPL");

#define MAX_TIMERS 1
#define MYTIMER_MAJOR 61

static struct fasync_struct *async_queue = NULL;
static struct proc_dir_entry *proc_entry;

/* Major number */
static int mytimer_major = MYTIMER_MAJOR;
static int max_active_timers = 1; // default, change with -m flag
static char message_type = '\0'; // U for updated timer, R for reached max timers, \0 for neither
static bool list_timers = false; // flag for listing timers
static bool timer_expired = false; // flag for expired timer
static bool max_timers_reached = false; // flag for reached max timers (for printing purposes)
static int update_timer_index; // index of timer to update
static long module_start_time; // in jiffies, for calculating uptime

typedef struct {
	struct timer_list timer;
	int seconds;
	char message[128];
	unsigned long start_time; // in jiffies, for calculating expiration time
	bool active;
	pid_t pid; // process ID of requester
	char comm[TASK_COMM_LEN]; // command name of requester
} mytimer_t;

// static mytimer_t * timers;
static mytimer_t timers[MAX_TIMERS];

static int mytimer_open(struct inode *inode, struct file *filp);
static int mytimer_release(struct inode *inode, struct file *filp);
static ssize_t mytimer_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t mytimer_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
static int mytimer_fasync(int fd, struct file *filp, int mode);
static int mytimer_proc_open(struct inode *inode, struct file *file);
static int mytimer_proc_show(struct seq_file *m, void *v);
static int mytimer_init(void);
static void mytimer_exit(void);

/* Structure for file access operations */
struct file_operations mytimer_fops = {
	.open = mytimer_open,
	.release = mytimer_release,
	.read = mytimer_read,
	.write = mytimer_write,
	.fasync = mytimer_fasync
};

static const struct file_operations mytimer_proc_fops = {
    .owner = THIS_MODULE,
    .open = mytimer_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

module_init(mytimer_init);
module_exit(mytimer_exit);

static void mytimer_callback(struct timer_list *t)
{
    mytimer_t *mytimer = from_timer(mytimer, t, timer);
    // printk(KERN_INFO "%s\n", mytimer->message); // temp
    if (async_queue) {
        kill_fasync(&async_queue, SIGIO, POLL_IN);
	}
	timer_expired = true;
	mytimer->active = false;
}

static int mytimer_init(void) {
	// register device file
	int result;
	result = register_chrdev(mytimer_major, "mytimer", &mytimer_fops);
	if (result < 0) {
		return result;
	}

	// init proc entry
	proc_entry = proc_create("mytimer", 0644, NULL, &mytimer_proc_fops);
	if (proc_entry == NULL) {
		unregister_chrdev(mytimer_major, "mytimer");
		return -ENOMEM;
	}

	// init timers
	memset(timers, 0, sizeof(timers));

	module_start_time = jiffies; // set start time to current time (in jiffies)
	return 0;
}

static void mytimer_exit(void) {
	int i;
	// printk(KERN_ALERT "Removing mytimer module\n");

	// free timers
	for (i = 0; i < max_active_timers; i++) {
        // if (timers[i].active) {
            del_timer_sync(&timers[i].timer);  // Ensures the timer is fully stopped
            memset(&timers[i], 0, sizeof(mytimer_t));
        // }
    }

	if (proc_entry) {
		remove_proc_entry("mytimer", NULL);
	}

	if (async_queue) {
        kill_fasync(&async_queue, SIGIO, POLL_HUP);
        async_queue = NULL; 
    }

	// unregister device
	unregister_chrdev(mytimer_major, "mytimer");
}

static int mytimer_open(struct inode *inode, struct file *filp) {
	// printk(KERN_INFO "open called: process id %d, command %s\n",
	// 		current->pid, current->comm);
	return 0;
}

static int mytimer_release(struct inode *inode, struct file *filp) {
	// printk(KERN_INFO "release called: process id %d, command %s\n",
	// 		current->pid, current->comm);
	mytimer_fasync(-1, filp, 0);
	return 0;
}

// handle all file reads from userspace
static ssize_t mytimer_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
	int i;
	char tbuf[256], *tbptr = tbuf; // temporary buffer + pointer
	size_t len;

	// printk("mytimer_read called\n"); // temp

	if (*f_pos > 0) {
		*f_pos = 0; // reset file position
    }

	// printk("message type: %c\n", message_type); // temp
	// printk("timer expired: %d\n", timer_expired); // temp
	// printk("max timers reached: %d\n", max_timers_reached); // temp
	// printk("list timers: %d\n", list_timers); // temp
	// printk("update timer index: %d\n", update_timer_index); // temp
	/*************************************************************/
	// check if timer expired
	if (timer_expired) {
		// print message to stdout
		// printk(KERN_INFO "printing timer expired\n"); // temp
		tbptr += sprintf(tbptr, "%s\n", timers[0].message);
		len = tbptr - tbuf; // length of string in temporary buffer

		// limit count to prevent buffer overflows
		if (count > len - *f_pos) {
			count = len - *f_pos;
		}

		// copy to user, check for errors
		if (copy_to_user(buf, tbuf + *f_pos, count)) {
			return -EFAULT;
		}

		*f_pos += count; // increment file position
		timer_expired = false; // reset flag
		return count;
	}

	if (max_timers_reached) {
		// printk(KERN_INFO "printing max timers reached\n"); // temp
		tbptr += sprintf(tbptr, "Error: multiple timers not supported.\n");
		len = tbptr - tbuf; // length of string in temporary buffer

		// limit count to prevent buffer overflows
		if (count > len - *f_pos) {
			count = len - *f_pos;
		}

		// copy to user, check for errors
		if (copy_to_user(buf, tbuf + *f_pos, count)) {
			return -EFAULT;
		}

		*f_pos += count; // increment file position
		max_timers_reached = false; // reset flag
		return count;
	}

	// for -s print messages
	if (!list_timers) {	
		if (message_type == 'U') {
			// printk(KERN_INFO "updated timer\n"); // temp

			tbptr += sprintf(tbptr, "The timer %s was updated\n", timers[update_timer_index].message);

            // printk("The timer %s was updated", timers[update_timer_index].message); // temp
		} else if (message_type == 'R') {
			// printk(KERN_INFO "max timers reached\n"); // temp
			tbptr += sprintf(tbptr, "Cannot add another timer!\n");
		} else {
            list_timers = false; // temp?
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
	// printk(KERN_INFO "listing timers\n"); // temp
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

    // printk("Is -l\n"); // temp

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

	// printk("mytimer_write called\n"); // temp

	// copy from user, check for errors
	if (copy_from_user(kbuf, buf, count)) {
		return -EFAULT;
	}

	kbuf[count] = '\0'; // null terminate string
	list_timers = false;
	max_timers_reached = false; // flag for printing purposes

	// check for set NEW timer (-s)
	if (kbuf[0] == 's' && sscanf(kbuf, "s %d %127[^\n]", &sec, msg) == 2) {
		// printk(KERN_INFO "setting new timer detected\n"); // temp
		for (i = 0; i < max_active_timers; i++) {
			if (!timers[i].active) {
				strcpy(timers[i].message, msg);
                timers[i].seconds = sec;
				timers[i].start_time = jiffies; // set start time to current time (in jiffies)
                timers[i].active = true;
                timer_setup(&timers[i].timer, mytimer_callback, 0);
                mod_timer(&timers[i].timer, jiffies + sec * HZ);

				timers[i].pid = current->pid;
            	get_task_comm(timers[i].comm, current);

				return count;
			}
		}

		if (i == max_active_timers) {
			// set message type to reached max timers
			// printk(KERN_INFO "max timers reached\n"); // temp
			message_type = 'R';
			return count;
		}

	} else if (kbuf[0] == 'u' && sscanf(kbuf, "u %d %127[^\n]", &sec, msg) == 2) {
		// printk(KERN_INFO "updating timer detected\n"); // temp
		// update existing timer
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
			}
		}
	} else if (kbuf[0] == 'r') { // remove all timers (-r)
		// printk(KERN_INFO "removing all timers detected\n"); // temp
		// remove all active timers
		for (i = 0; i < max_active_timers; i++) {
			if (timers[i].active) {
				del_timer_sync(&timers[i].timer);  // Ensures the timer is fully stopped
				timers[i].active = false;
			}
		}
		return count;
	} else if (kbuf[0] == 'm' && sscanf(kbuf, "m %d", &new_max) == 1) { // set max active timer count (-m)
		// change max active timer count to given [COUNT]
		// check for valid input
		if (new_max < 1 || new_max > MAX_TIMERS) {
			max_timers_reached = true;
			return -EINVAL;
		} else {
			max_timers_reached = false;
			max_active_timers = new_max;
			return count;
		}
		
	} else if (kbuf[0] == 'l') { // list active timers (-l)
		// printk(KERN_INFO "listing timers detected\n"); // temp
		list_timers = true;
		return count;
	}

	// final checks
	// if (kbuf[0] != 'u' && kbuf[0] != 's') {
	// 	message_type = '\0';
	// }
	if (kbuf[0] != 'l') {
		list_timers = false;
	}
	if (kbuf[0] != 'm') {
		max_timers_reached = false;
	}
	
	return -EINVAL;
}

static int mytimer_fasync(int fd, struct file *filp, int mode)
{
    return fasync_helper(fd, filp, mode, &async_queue);
}

static int mytimer_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, mytimer_proc_show, NULL);
}

static int mytimer_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "MODULE_NAME: mytimer\n");
    // seq_printf(m, "%lld\n", uptime.tv_sec * 1000 + uptime.tv_nsec / 1000000); // convert to milliseconds TEMP
	seq_printf(m, "MSEC: %ld\n", (jiffies - module_start_time) * 1000 / HZ); // convert to milliseconds

    if (timers[0].active) {  
		// calculate seconds remaining
		unsigned long elapsed_jiffies = jiffies - timers[0].start_time;
		unsigned long elapsed_seconds = elapsed_jiffies / HZ;
		int expiration_time = timers[0].seconds - elapsed_seconds;

		if (expiration_time < 0) {
			expiration_time = 0; // ensure non-negative
		}
		seq_printf(m, "PID: %d\n", timers[0].pid);   
        seq_printf(m, "CMD: %s\n", timers[0].comm); 
		seq_printf(m, "SEC: %d\n", expiration_time);
	}
    return 0;
}
