#ifndef _TLSF_COMPAT_H_
#define _TLSF_COMPAT_H_

#ifndef pr_err
#define pr_err(fmt, arg...) \
	printk(KERN_ERR fmt, ##arg)
#endif

#ifndef pr_warning
#define pr_warning(fmt, arg...) \
	printk(KERN_WARNING fmt, ##arg)
#endif

#ifndef pr_info
#define pr_info(fmt, arg...) \
	printk(KERN_INFO fmt, ##arg)
#endif

#ifndef pr_debug
#define pr_debug(fmt, arg...) \
	printk(KERN_DEBUG fmt, ##arg)
#endif

#endif

