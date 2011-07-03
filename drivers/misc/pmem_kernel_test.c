/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/module.h>
#include <linux/android_pmem.h>
#include <linux/io.h>
#include <linux/miscdevice.h>

#define MODULE_NAME "pmem_kernel_test"

#define ALIGNMENT_TESTMASK_4K ((1 << 12) - 1)
#define ALIGNMENT_TESTMASK_1M ((1 << 20) - 1)

/* use #defines here instead of static inline so __func__ is right */
#define OUTPUT_FUNCTION_SUCCESS \
	printk(KERN_INFO MODULE_NAME ": %s success\n", __func__)

#define OUTPUT_FUNCTION_FAILURE(ret_val) \
	printk(KERN_INFO MODULE_NAME ": %s FAILS, ret %d\n", __func__, ret)

#define OUTPUT_FINAL_FUNCTION_STATUS(ret) \
	{ \
		if (ret) \
			OUTPUT_FUNCTION_FAILURE(ret); \
		else \
			OUTPUT_FUNCTION_SUCCESS;\
	}

int pmem_kernel_test_run_tests_on_init;
EXPORT_SYMBOL(pmem_kernel_test_run_tests_on_init);

MODULE_PARM_DESC(pmem_kernel_test_run_tests_on_init,
	"Do all tests at startup if set; always wait for IOCTL "
	"commands");

module_param_named(run_tests_on_init,
		pmem_kernel_test_run_tests_on_init,
		bool,
		S_IRUGO | S_IWUSR);

#define NUM_DYN_ALLOCED_BUFFERS 512

static int read_write_test(void *kernel_addr, unsigned long size)
{
	int j, *p;

	printk(KERN_INFO MODULE_NAME ": %s entry, kernel_addr %p, size %lu\n",
		__func__, kernel_addr, size);

	for (j = 0, p = kernel_addr;
			j < (size / sizeof(int));
			j++, p++)
		*p = j;

	for (j = 0, p = kernel_addr;
			j < (size / sizeof(int));
			j++, p++)
		if (*p != j) {
			printk(KERN_INFO MODULE_NAME ": %s FAILS @ int offset"
				" %d!\n", __func__, j);
			return -1;
		}

	OUTPUT_FUNCTION_SUCCESS;
	return 0;
}

static void map_and_check(int32_t physaddr, unsigned long size)
{
	void *kernel_addr = ioremap((unsigned long)physaddr, size);

	printk(KERN_INFO MODULE_NAME ": %s, physaddr %#x, size %lu\n",
		__func__, physaddr, size);

	if (!kernel_addr) {
		printk(KERN_INFO MODULE_NAME ": %s FAILS ioremap!\n",
			__func__);
	} else {
		if (read_write_test(kernel_addr, size))
			printk(KERN_INFO MODULE_NAME ": %s kernel_addr %p "
				"FAILS read_write_test!\n",
				__func__, kernel_addr);
		else
			printk(KERN_INFO MODULE_NAME ": %s kernel_addr %p "
				"read_write_test success\n",
				__func__, kernel_addr);

		iounmap(kernel_addr);

		OUTPUT_FUNCTION_SUCCESS;
	}

}

int32_t alloc_aligned_test(size_t size, int32_t flags)
{
	int32_t ret = pmem_kalloc(size, flags);

	printk(KERN_INFO MODULE_NAME ": %s, size %u, flags %#x\n",
		__func__, size, flags);

	if (ret <= 0) { /* 0 while technically legal is bad news here! */
		printk(KERN_INFO MODULE_NAME "%s pmem_kalloc FAILS %d\n",
			__func__, ret);
	} else {
		int32_t alignment_testmask;
		char *alignment_output_str;
		switch (flags & PMEM_ALIGNMENT_MASK) {
		case PMEM_ALIGNMENT_4K:
			alignment_testmask = ALIGNMENT_TESTMASK_4K;
			alignment_output_str = "4K";
			break;
		case PMEM_ALIGNMENT_1M:
			alignment_testmask = ALIGNMENT_TESTMASK_4K;
			alignment_output_str = "1M";
			break;
		default:
			printk(KERN_INFO MODULE_NAME "%s internal error - "
				"invalid alignment flag %#x\n",
				__func__, (flags & PMEM_ALIGNMENT_MASK));
			pmem_kfree(ret); /* this may not work! */
			return -1;
		}

		if (ret & alignment_testmask) {
			printk(KERN_INFO MODULE_NAME
				": %s %s alignment returned bad alignment",
				__func__, alignment_output_str);
			pmem_kfree(ret); /* this may not work! */
			return -1;
		}
		OUTPUT_FUNCTION_SUCCESS;
	}
	return ret;
}

static int test_pmem_device(size_t size, int32_t flags)
{
	int32_t ret;

	printk(KERN_INFO MODULE_NAME ": %s entry, size %d, flags %#x\n",
		__func__, size, flags);

	ret = alloc_aligned_test(size, flags);
	if (ret < 0) {
		printk(KERN_INFO MODULE_NAME
			": %s alloc_aligned_test FAILS %d\n",
			__func__, ret);
		return (int)ret;
	} else {
		printk(KERN_INFO MODULE_NAME
			": %s alloc_aligned_test success\n", __func__);
	}

	map_and_check(ret, (unsigned long)size);

	ret = pmem_kfree(ret);
	if (ret < 0)
		printk(KERN_INFO MODULE_NAME ": %s pmem_kfree FAILS %d\n",
			__func__, ret);
	else
		printk(KERN_INFO MODULE_NAME ": %s pmem_kfree success\n",
			__func__);

	OUTPUT_FUNCTION_SUCCESS;
	return (int)ret;
}

static int test_pmem_device_n1(int32_t flags)
{
	int ret;

	printk(KERN_INFO MODULE_NAME ": %s entry, flags %#x\n",
		__func__, flags);

	ret = test_pmem_device(0x100000, flags);
	if (ret < 0)
		printk(KERN_INFO MODULE_NAME ": %s FAILS %d\n",
			__func__, ret);
	else
		OUTPUT_FUNCTION_SUCCESS;

	return ret;
}

static int test_pmem_device_n2(int32_t flags)
{
	int ret;

	printk(KERN_INFO MODULE_NAME ": %s entry, flags %#x\n",
		__func__, flags);

	ret = test_pmem_device(0x200000, flags);
	if (ret < 0)
		printk(KERN_INFO MODULE_NAME ": %s FAILS %d\n",
			__func__, ret);
	else
		OUTPUT_FUNCTION_SUCCESS;

	return ret;
}

static int test_pmem_device_a1(int32_t flags)
{
	int32_t ret1, ret2;
	int ret = 0;
	void *kernel_addr1, *kernel_addr2;

	printk(KERN_INFO MODULE_NAME "%s entry, flags %#x\n",
		__func__, flags);

	ret1 = alloc_aligned_test(0x100000, flags);
	if (ret1 < 0) {
		printk(KERN_INFO MODULE_NAME
			": %s alloc_aligned_test 0x100000 FAILS %d\n",
			__func__, ret1);
		ret = ret1;
		goto leave1;
	} else {
		printk(KERN_INFO MODULE_NAME
			": %s alloc_aligned_test 0x100000 success\n",
			__func__);
	}

	kernel_addr1 = ioremap((unsigned long)ret1, 0x100000);
	if (!kernel_addr1) {
		printk(KERN_INFO MODULE_NAME
			": %s ioremap for 0x100000 FAILS\n",
			__func__);
		ret = -1;
		goto leave2;
	} else {
		printk(KERN_INFO MODULE_NAME
			": %s ioremap for 0x100000 success\n",
			__func__);
	}

	if (read_write_test(kernel_addr1, 0x100000)) {
		printk(KERN_INFO MODULE_NAME
			": %s 0x100000 read_write_test FAILS\n",
			__func__);
		ret = -1;
		goto leave3;
	} else {
		printk(KERN_INFO MODULE_NAME
			": %s 0x100000 read_write_test success\n",
			__func__);
	}

	ret2 = alloc_aligned_test(0x10000, flags);
	if (ret2 < 0) {
		printk(KERN_INFO MODULE_NAME
			": %s alloc_aligned_test 0x10000 FAILS %d\n",
			__func__, ret2);
		ret = ret2;
		goto leave3;
	} else {
		printk(KERN_INFO MODULE_NAME
			": %s alloc_aligned_test 0x10000 success\n",
			__func__);
	}

	kernel_addr2 = ioremap((unsigned long)ret2, 0x10000);
	if (!kernel_addr2) {
		printk(KERN_INFO MODULE_NAME
			": %s ioremap for 0x10000 FAILS\n", __func__);
		ret = -1;
		goto leave4;
	} else {
		printk(KERN_INFO MODULE_NAME
			": %s ioremap for 0x10000 success\n", __func__);
	}

	if (read_write_test(kernel_addr2, 0x10000)) {
		printk(KERN_INFO MODULE_NAME
			": %s 0x10000 read_write_test FAILS\n",
			__func__);
		ret = -1;
		goto leave5;
	} else {
		printk(KERN_INFO MODULE_NAME
			": %s 0x10000 read_write_test success\n",
			__func__);
	}

	OUTPUT_FUNCTION_SUCCESS;

leave5:
	iounmap(kernel_addr2);
leave4:
	pmem_kfree(ret2);
leave3:
	iounmap(kernel_addr1);
leave2:
	pmem_kfree(ret1);
leave1:
	return ret;
}

static int nominal_test(void)
{
	int ret;

	printk(KERN_INFO MODULE_NAME "%s entry\n", __func__);

	/* test_pmem_device_n1, for all different alignments and memtypes */
	/* make sure error case fails */
	ret = test_pmem_device_n1(0);
	if (!ret) { /* !! bad news, this should fail! */
		printk(KERN_INFO MODULE_NAME
			": unexpected %s n1 success for zero flags!\n",
			__func__);
		ret = -EFAULT;
		goto done;
	} else {
		printk(KERN_INFO MODULE_NAME
			": correct %s n1 failure of error case %d\n",
			__func__, ret);
	}

	ret = test_pmem_device_n1(PMEM_MEMTYPE_EBI1 | PMEM_ALIGNMENT_4K);
	if (ret) { /* now, if we fail, bad news */
		printk(KERN_INFO MODULE_NAME
			": %s n1 4K alignment FAILS, ret %d\n",
			__func__, ret);
		goto done;
	} else {
		printk(KERN_INFO MODULE_NAME ": %s n1 4K align success\n",
			__func__);
	}

	ret = test_pmem_device_n1(PMEM_MEMTYPE_EBI1 | PMEM_ALIGNMENT_1M);
	if (ret) { /* now, if we fail, bad news */
		printk(KERN_INFO MODULE_NAME
			": %s n1 1M alignment FAILS, ret %d\n",
			__func__, ret);
		goto done;
	} else {
		printk(KERN_INFO MODULE_NAME ": %s n1 1M align success\n",
			__func__);
	}

	/* test_pmem_device_n2, for all different alignments and memtypes */
	/* make sure error case fails */
	ret = test_pmem_device_n2(0);
	if (!ret) { /* !! bad news, this should fail! */
		printk(KERN_INFO MODULE_NAME
			": unexpected %s n2 success for zero flags!\n",
			__func__);
		ret = -ENODEV;
		goto done;
	} else {
		printk(KERN_INFO MODULE_NAME
			": correct %s n2 failure of error case %d\n",
			__func__, ret);
	}

	ret = test_pmem_device_n2(PMEM_MEMTYPE_EBI1 | PMEM_ALIGNMENT_4K);
	if (ret) { /* now, if we fail, bad news */
		printk(KERN_INFO MODULE_NAME
			": %s n2 4K alignment FAILS, ret %d\n",
			__func__, ret);
		goto done;
	} else {
		printk(KERN_INFO MODULE_NAME ": %s n2 4K align success\n",
			__func__);
	}

	ret = test_pmem_device_n2(PMEM_MEMTYPE_EBI1 | PMEM_ALIGNMENT_1M);
	if (ret) { /* now, if we fail, bad news */
		printk(KERN_INFO MODULE_NAME
			": %s n2 1M alignment FAILS, ret %d\n",
			__func__, ret);
		goto done;
	} else {
		printk(KERN_INFO MODULE_NAME ": %s n2 1M align success\n",
			__func__);
	}
done:
	OUTPUT_FINAL_FUNCTION_STATUS(ret);
	return 0;
}

static int adversarial_test(void)
{
	int ret;

	/* test_pmem_device_a1, for all different alignments and memtypes */
	printk(KERN_INFO MODULE_NAME "%s entry\n", __func__);

	/* make sure error case fails */
	ret = test_pmem_device_a1(0);
	if (!ret) { /* !! bad news, this should fail! */
		printk(KERN_INFO MODULE_NAME
			": unexpected %s a1 success for zero flags!\n",
			__func__);
		ret = -EFAULT;
		goto done;
	} else {
		printk(KERN_INFO MODULE_NAME
			"correct %s a1 failure of error case %d\n",
			__func__, ret);
	}

	ret = test_pmem_device_a1(PMEM_MEMTYPE_EBI1 | PMEM_ALIGNMENT_4K);
	if (ret) { /* now, if we fail, bad news */
		printk(KERN_INFO MODULE_NAME
			": %s a1 4K alignment FAILS, ret %d\n",
			__func__, ret);
		goto done;
	} else {
		printk(KERN_INFO MODULE_NAME
			": %s a1 4K alignment success\n",
			__func__);
	}

	ret = test_pmem_device_a1(PMEM_MEMTYPE_EBI1 | PMEM_ALIGNMENT_1M);
	if (ret) { /* now, if we fail, bad news */
		printk(KERN_INFO MODULE_NAME
			": %s a1 1M alignment FAILS, ret %d\n",
			__func__, ret);
		goto done;
	} else {
		printk(KERN_INFO MODULE_NAME
			": %s a1 1M alignment success\n",
			__func__);
	}

done:
	OUTPUT_FINAL_FUNCTION_STATUS(ret);
	return ret;
}

static int huge_allocation_test(void)
{
	int ret;

	/* Huge allocation test */
	ret = alloc_aligned_test(0x70000000,
		PMEM_MEMTYPE_EBI1 | PMEM_ALIGNMENT_1M);
	if (ret >= 0) { /* !! bad news, this should fail! */
		pmem_kfree(ret);
		printk(KERN_INFO MODULE_NAME
			": %s unexpected success for huge allocation test!\n",
			__func__);
		ret = -EFAULT;
	} else {
		printk(KERN_INFO MODULE_NAME
			": %s correct failure of huge allocation test error "
			"case %d\n",
			__func__, ret);
		ret = 0;
	}
	OUTPUT_FINAL_FUNCTION_STATUS(ret);
	return ret;
}

static int free_of_unallocated_test(void)
{
	int ret;

	/* free of unallocated pointer */
	ret = pmem_kfree(0x32746876);
	if (ret >= 0) { /* !! bad news, this should fail! */
		printk(KERN_INFO MODULE_NAME
			": %s unexpected success for free of unallocated "
			"pointer!\n", __func__);
		ret = -EFAULT;
	} else {
		printk(KERN_INFO MODULE_NAME
			": %s correct failure of free of unallocated "
			"pointer error case %d\n",
			__func__, ret);
		ret = 0;
	}
	OUTPUT_FINAL_FUNCTION_STATUS(ret);
	return 0;
}

static int large_number_of_regions_test(void)
{
	int ret, i;
	static int32_t dyn_alloced[NUM_DYN_ALLOCED_BUFFERS];

	/* large number of regions */
	for (i = 0; i < NUM_DYN_ALLOCED_BUFFERS; i++) {
		ret = alloc_aligned_test(PAGE_SIZE,
			PMEM_MEMTYPE_EBI1 | PMEM_ALIGNMENT_4K);
		if (ret <= 0) {
			int j = i - 1;

			printk(KERN_INFO MODULE_NAME
				": %s Large number of regions test; "
				"allocate page #%d, ret %d FAILS\n",
				__func__, i, ret);

			if (!ret) /* extremely weird error */
				pmem_kfree(0);

			while (j >= 0)
				pmem_kfree(dyn_alloced[j--]);
			ret = -EFAULT;
			goto done;
		} else {
			dyn_alloced[i] = ret;
		}
	}

	ret = 0;
	for (i = 511; i >= 0; i--) {
		int32_t local_ret = pmem_kfree(dyn_alloced[i]);

		if (local_ret < 0) {
			ret = local_ret;
			printk(KERN_INFO MODULE_NAME
				": %s Large number of regions test; free page"
				" #%d, address %#x, ret %d FAILS\n",
				__func__, i, dyn_alloced[i], ret);
			ret = -EFAULT;
		}
	}
done:
	OUTPUT_FINAL_FUNCTION_STATUS(ret);
	return ret;
}

static long pmem_kernel_test_ioctl(struct file *ignored1,
		unsigned int cmd, unsigned long ignored2)
{
	switch (cmd) {
	case PMEM_KERNEL_TEST_NOMINAL_TEST_IOCTL:
		return nominal_test();
	case PMEM_KERNEL_TEST_ADVERSARIAL_TEST_IOCTL:
		return adversarial_test();
	case PMEM_KERNEL_TEST_HUGE_ALLOCATION_TEST_IOCTL:
		return huge_allocation_test();
	case PMEM_KERNEL_TEST_FREE_UNALLOCATED_TEST_IOCTL:
		return free_of_unallocated_test();
	case PMEM_KERNEL_TEST_LARGE_REGION_NUMBER_TEST_IOCTL:
		return large_number_of_regions_test();
	default:
		printk(KERN_ERR MODULE_NAME
			": %s, invalid command %#x\n",
			__func__, cmd);
		return -EINVAL;
	}
}

static const struct file_operations pmem_kernel_test_fops = {
	.unlocked_ioctl = pmem_kernel_test_ioctl,
};

static struct miscdevice pmem_kernel_test_miscdevice = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = MODULE_NAME,
	.fops = &pmem_kernel_test_fops,
	.parent = NULL,
};

static int __init pmem_kernel_test_init(void)
{
	int ret = misc_register(&pmem_kernel_test_miscdevice);
	if (ret) {
		printk(KERN_ERR MODULE_NAME ": failed to register misc "
		       "device, err: %d!\n", ret);
		goto out;
	}

	if (!pmem_kernel_test_run_tests_on_init) {
		printk(KERN_INFO MODULE_NAME ": not performing any tests at "
			"init.\n");
		goto out;
	}

	/* tests lifted directly from
	 * vendor/qcom-proprietary/kernel-tests/_pmem_test.c and
	 * ported to the kernel API.
	 * Full release test.
	 */
	ret = nominal_test();
	if (ret)
		goto done;

	ret = adversarial_test();
	if (ret)
		goto done;

	ret = huge_allocation_test();
	if (ret)
		goto done;

	ret = free_of_unallocated_test();
	if (ret)
		goto done;

	ret = large_number_of_regions_test();
	if (ret)
		goto done;

done:
	if (!ret)
		printk(KERN_INFO MODULE_NAME ": All PMEM kernel API tests "
			"PASS!\n");

out:
	return ret;
}

static void __exit pmem_kernel_test_exit(void)
{
	misc_deregister(&pmem_kernel_test_miscdevice);
}

device_initcall(pmem_kernel_test_init);
module_exit(pmem_kernel_test_exit);

MODULE_LICENSE("GPL v2");
