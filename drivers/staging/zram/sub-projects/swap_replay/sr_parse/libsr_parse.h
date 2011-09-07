#ifndef _LIB_SR_PARSE_H_
#define _LIB_SR_PARSE_H_

typedef void (swap_read_fn)(unsigned long index);
typedef void (swap_write_fn)(unsigned long index,
			unsigned long len, const unsigned char *hash);

/*
 * Arg:
 *	0: Success
 *	>0: Error: partial read
 *	<0: Error: file read error
 */
typedef void (replay_end_fn)(int err);

void * sr_gethandle(void);
void sr_puthandle(void *);
int set_sr_file(void *, const char *);
int set_sr_callbacks(void *, swap_read_fn, swap_write_fn, replay_end_fn);
int sr_start(void *);

#endif
