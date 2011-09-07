#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/relay.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/vmalloc.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>

#define K(x)	((x) << 10)
#define M(x)	((x) << 20)

#define MAX_SWAP_SIZE		M(64)
#define RELAY_BUFFER_SIZE	PAGE_SIZE
#define RELAY_NUM_BUFFERS	16

static void **table;
static struct proc_dir_entry *proc;
static char buffer[100];
static unsigned long drop_count;
static struct rchan *relay;
static spinlock_t write_lock;

unsigned long used_size, count;

static struct dentry *kt_create_buf_file_callback(const char *filename,
                                                        struct dentry *parent,
                                                        int mode,
                                                        struct rchan_buf *buf,
                                                        int *is_global)
{
	*is_global = 1;
	return debugfs_create_file(filename, mode, parent, buf,
				&relay_file_operations);
}

static int kt_remove_buf_file_callback(struct dentry *dentry)
{
	debugfs_remove(dentry);
	return 0;
}

static int kt_subbuf_start_callback(struct rchan_buf *buf,
				void *subbuf,
				void *prev_subbuf,
				size_t prev_padding)
{
	if (relay_buf_full(buf)) {
		pr_info("Dropped event count=%lu\n", ++drop_count);
		return 0;
	}

	return 1;
}

static struct rchan_callbacks kt_relay_callbacks = {
	.subbuf_start		= kt_subbuf_start_callback,
	.create_buf_file	= kt_create_buf_file_callback,
	.remove_buf_file	= kt_remove_buf_file_callback,
};

static int proc_write(struct file *filp, const char __user *buff,
			unsigned long len, void *data)
{
	unsigned int page_no = 0, clen = 0;

	if (copy_from_user(&buffer, buff, len))
		return -EFAULT;
	sscanf(buffer, "%u %u", &page_no, &clen);

	if (table[page_no]) {
		used_size -= (unsigned long)ksize(table[page_no]);
		kfree(table[page_no]);
	}
	if (!(table[page_no] = kmalloc(clen, GFP_KERNEL))) {
		printk(KERN_INFO "Error allocating mem for page: %u\n", page_no);
	} else {
		used_size += (unsigned long)ksize(table[page_no]);
	}

	count++;
	sprintf(buffer, "%lu %lu %lu\n", count, used_size, used_size >> 10);

        relay_write(relay, buffer, strlen(buffer));

	return len;
}

static int setup_relay(void)
{
	relay = relay_open("kmem_cache_relay", NULL, RELAY_BUFFER_SIZE,
			RELAY_NUM_BUFFERS, &kt_relay_callbacks, NULL);
	if (!relay) {
		pr_err("Error creating relay channel\n");
		return -ENOMEM;
	}
	return 0;
}

static int __init kt_init(void)
{
	table = vmalloc((MAX_SWAP_SIZE / PAGE_SIZE) * sizeof(*table));
	if (table == NULL)
		return -ENOMEM;
	memset(table, 0, (MAX_SWAP_SIZE / PAGE_SIZE) * sizeof(*table));

	proc = create_proc_entry("kmem_cache_test", 0666, NULL);
	if (proc == NULL) {
		vfree(table);
		return -ENOMEM;
	}
	proc->write_proc = proc_write;
	spin_lock_init(&write_lock);
	return setup_relay();
}

static void __exit kt_exit(void)
{
	unsigned long entry;
	remove_proc_entry("kmalloc_test", NULL);
        relay_close(relay);
	for (entry = 0; entry < MAX_SWAP_SIZE / PAGE_SIZE; entry++)
		if (table[entry])
			kfree(table[entry]);
	vfree(table);
}

MODULE_LICENSE("GPL");

module_init(kt_init);
module_exit(kt_exit);

