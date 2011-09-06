#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/relay.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/pgtable.h>
#include <asm/uaccess.h>

#include "../../allocators/tlsf-kmod/tlsf.h"

#define K(x)	(x << 10)
#define M(x)	(x << 20)

#define MAX_SWAP_SIZE	M(64)
#define INIT_SIZE	K(16)
#define GROW_SIZE	K(16)

#define RELAY_BUFFER_SIZE       PAGE_SIZE
#define RELAY_NUM_BUFFERS       16

static void *mem_pool;
static void **table;
static struct proc_dir_entry *proc;
static char buffer[100];
static unsigned long count, drop_count;

static struct rchan *relay;
static spinlock_t write_lock;

static void *get_mem(size_t size)
{
        return __vmalloc(size, GFP_NOIO, PAGE_KERNEL);
}

static void put_mem(void *ptr)
{
	vfree(ptr);
}

static struct dentry *tk_create_buf_file_callback(const char *filename,
                                                        struct dentry *parent,
                                                        int mode,
                                                        struct rchan_buf *buf,
                                                        int *is_global)
{
	*is_global = 1;
	return debugfs_create_file(filename, mode, parent, buf,
		&relay_file_operations);
}

static int tk_remove_buf_file_callback(struct dentry *dentry)
{
	debugfs_remove(dentry);
	return 0;
}

static int tk_subbuf_start_callback(struct rchan_buf *buf,
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

static struct rchan_callbacks tk_relay_callbacks = {
	.subbuf_start		= tk_subbuf_start_callback,
	.create_buf_file	= tk_create_buf_file_callback,
	.remove_buf_file	= tk_remove_buf_file_callback,
};

static ssize_t proc_write(struct file *filp, const char __user *buff,
			unsigned long len, void *data)
{
	unsigned int page_no = 0, clen = 0;
	unsigned long used_size;
	
	if (copy_from_user(&buffer, buff, len)) {
		pr_info("Error copying buffer from user: len=%lu\n", len);
		return -EFAULT;
	}
	//pr_info("buffer=[%s]\n", buffer);
	sscanf(buffer, "%u %u", &page_no, &clen);

	//temp
	//buffer[len] = '\0';

	if (table[page_no])
		tlsf_free(table[page_no], mem_pool);
	if (!(table[page_no] = tlsf_malloc(clen, mem_pool)))
		pr_info("Error allocating mem for page: %u\n", page_no);
	used_size = (unsigned long)tlsf_get_used_size(mem_pool);

	count++;
        //spin_lock(&write_lock);
	//sprintf(buffer, "%lu\n", used_size);
	sprintf(buffer, "%lu %lu %lu\n", count, used_size, used_size >> 10);
	relay_write(relay, buffer, strlen(buffer));
        //relay_write(relay, &used_size, sizeof(unsigned long));
        //spin_unlock(&write_lock);
	return len;
}

static int setup_relay(void)
{
	relay = relay_open("tlsf_relay", NULL, RELAY_BUFFER_SIZE,
		RELAY_NUM_BUFFERS, &tk_relay_callbacks, NULL);
	if (!relay) {
		pr_err("Error creating relay channel\n");
		return -ENOMEM;
	}
	return 0;
}

static int __init tlsf_kmod_init(void)
{
	table = vmalloc((MAX_SWAP_SIZE / PAGE_SIZE) * sizeof(*table));
	if (table == NULL)
		return -ENOMEM;
	memset(table, 0, (MAX_SWAP_SIZE / PAGE_SIZE) * sizeof(*table));

	mem_pool = tlsf_create_memory_pool("tlsf_kmod_test", get_mem, put_mem,
			INIT_SIZE, 0, GROW_SIZE);
	if (mem_pool == NULL) {
		vfree(table);
		return -ENOMEM;
	}

	proc = create_proc_entry("tlsf_test", 0666, NULL);
	if (proc == NULL) {
		tlsf_destroy_memory_pool(mem_pool);
		vfree(table);
		return -ENOMEM;
	}

	proc->write_proc = proc_write;
        spin_lock_init(&write_lock);

	return setup_relay();
}

static void __exit tlsf_kmod_exit(void)
{
	unsigned long entry;
	remove_proc_entry("tlsf_test", &proc_root);
	relay_close(relay);
	for (entry = 0; entry < MAX_SWAP_SIZE / PAGE_SIZE; entry++)
		if (table[entry])
			tlsf_free(table[entry], mem_pool);
	vfree(table);
	tlsf_destroy_memory_pool(mem_pool);
}

MODULE_LICENSE("GPL");

module_init(tlsf_kmod_init);
module_exit(tlsf_kmod_exit);
