#ifndef _KFS_H_
#define _KFS_H_

typedef struct file* ATH_OS_FD;

typedef struct _ATH_OS_FS_INFO_
{
	int				fsuid;
	int				fsgid;
	mm_segment_t	fs;
}ATH_OS_FS_INFO;

#define IS_FILE_OPEN_ERR(_fd) 	IS_ERR((_fd))

typedef unsigned char			BOOLEAN;

#ifndef TRUE
#define TRUE				1
#endif
#ifndef FALSE
#define FALSE				0
#endif

#define DBGPRINT(Level, Fmt)		\
{                                   \
    printk Fmt;                  	\
}

#endif/*_KFS_H_*/
