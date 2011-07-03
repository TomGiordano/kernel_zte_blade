/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __LIBRA_SDIOIF_H__
#define __LIBRA_SDIOIF_H__

/*
 * Header for SDIO Card Interface Functions
 */
#include <linux/kthread.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_func.h>

#define LIBRA_MAN_ID              0x70


int    libra_sdio_configure(sdio_irq_handler_t libra_sdio_rxhandler,
		void (*func_drv_fn)(int *status),
		u32 funcdrv_timeout, u32 blksize);
void   libra_sdio_deconfigure(struct sdio_func *func);
struct sdio_func *libra_getsdio_funcdev(void);
void   libra_sdio_setprivdata(struct sdio_func *sdio_func_dev,
		void *padapter);
void   *libra_sdio_getprivdata(struct sdio_func *sdio_func_dev);
void   libra_claim_host(struct sdio_func *sdio_func_dev,
		pid_t *curr_claimed, pid_t current_pid,
		atomic_t *claim_count);
void   libra_release_host(struct sdio_func *sdio_func_dev,
		pid_t *curr_claimed, pid_t current_pid,
		atomic_t *claim_count);
void   libra_sdiocmd52(struct sdio_func *sdio_func_dev,
		u32 addr, u8 *b, int write, int *err_ret);
u8     libra_sdio_readsb(struct sdio_func *func, void *dst,
		unsigned int addr, int count);
int    libra_sdio_memcpy_fromio(struct sdio_func *func,
		void *dst, unsigned int addr, int count);
int    libra_sdio_writesb(struct sdio_func *func,
		unsigned int addr, void *src, int count);
int    libra_sdio_memcpy_toio(struct sdio_func *func,
		unsigned int addr, void *src, int count);
int    libra_sdio_enable_polling(void);

#endif /* __LIBRA_SDIOIF_H__ */
