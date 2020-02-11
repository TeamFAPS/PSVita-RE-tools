/*
PSVita RE Tools: NetLoggingMgr aka PrincessLog
Copyright (C) 2020 Asakura Reiko

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "ringbuf.h"
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>

#define SCE_KERNEL_ATTR_SINGLE			(0x00000000U)	/**< 複数のスレッドが同時に待機できない(イベントフラグのみ) */
#define SCE_KERNEL_ATTR_MULTI			(0x00001000U)	/**< 複数のスレッドが同時に待機できる(イベントフラグのみ) */
#define SCE_KERNEL_ATTR_TH_FIFO			(0x00000000U)	/**< 待機スレッドのキューイングはFIFO */
#define SCE_KERNEL_ATTR_TH_PRIO			(0x00002000U)	/**< 待機スレッドのキューイングはスレッドの優先度順 */

#define SCE_KERNEL_MUTEX_ATTR_TH_FIFO			SCE_KERNEL_ATTR_TH_FIFO	/**< ミューテックスの待機スレッドのキューイングはFIFO */
#define SCE_KERNEL_MUTEX_ATTR_TH_PRIO			SCE_KERNEL_ATTR_TH_PRIO	/**< ミューテックスの待機スレッドのキューイングはスレッドの優先度順 */
#define SCE_KERNEL_MUTEX_ATTR_RECURSIVE			(0x00000002U)			/**< ミューテックスは再帰ロック可能 */
#define SCE_KERNEL_MUTEX_ATTR_CEILING			(0x00000004U)			/**< ミューテックスの優先度シーリング機能を使用する */

#define SCE_KERNEL_EVF_ATTR_TH_FIFO		SCE_KERNEL_ATTR_TH_FIFO		/**< イベントフラグの待機スレッドのキューイングはFIFO */
#define SCE_KERNEL_EVF_ATTR_TH_PRIO		SCE_KERNEL_ATTR_TH_PRIO		/**< イベントフラグの待機スレッドのキューイングはスレッドの優先度順 */
#define SCE_KERNEL_EVF_ATTR_SINGLE		SCE_KERNEL_ATTR_SINGLE	/**< 複数のスレッドが同時に待機できない */
#define SCE_KERNEL_EVF_ATTR_MULTI		SCE_KERNEL_ATTR_MULTI	/**< 複数のスレッドが同時に待機できる */

#define SCE_KERNEL_EVF_WAITMODE_AND			(0x00000000U)	/**< AND 待ち */
#define SCE_KERNEL_EVF_WAITMODE_OR			(0x00000001U)	/**< OR 待ち */
#define SCE_KERNEL_EVF_WAITMODE_CLEAR_ALL	(0x00000002U)	/**< 待ち成立後、すべてのビットをクリア  */
#define SCE_KERNEL_EVF_WAITMODE_CLEAR_PAT	(0x00000004U)	/**< 待ち成立後、bitPatternに指定したビットをクリア */

#define RINGBUF_EVF_NON_EMPTY 0x00000001

static SceUID evf_uid = -1;
static SceUID mtx_uid = -1;
static SceUID memblock_uid = -1;

static int buf_len = 0;
static char *base_ptr = NULL;
static char *get_ptr = NULL;
static char *put_ptr = NULL;

static int idx(char *ptr) {
	return (unsigned int)ptr % buf_len;
}

static void inc(char **ptr) {
	*ptr = base_ptr + idx(*ptr + 1);
}

static int empty(void) {
	return get_ptr == put_ptr;
}

static int full(void) {
	return idx(put_ptr + 1) == idx(get_ptr);
}

static int put(char c) {
	if (full()) {
		return -1;
	}
	*put_ptr = c;
	inc(&put_ptr);
	return 0;
}

static int put_clobber(char c) {
	if (full()) {
		inc(&get_ptr);
	}
	*put_ptr = c;
	inc(&put_ptr);
	return 0;
}

static int get(char *c) {
	if (empty()) {
		return -1;
	}
	*c = *get_ptr;
	inc(&get_ptr);
	return 0;
}

int ringbuf_init(int size) {
	int ret = 0;

	evf_uid = ksceKernelCreateEventFlag("RingBufferEventFlag",
		SCE_KERNEL_ATTR_TH_FIFO | SCE_KERNEL_EVF_ATTR_MULTI,
		0x00000000,
		NULL);
	if (evf_uid < 0) {
		ret = evf_uid;
		goto fail_evf;
	}

	mtx_uid = ksceKernelCreateMutex("RingBufferMutex", SCE_KERNEL_MUTEX_ATTR_TH_FIFO, 0, NULL);
	if (mtx_uid < 0) {
		ret = mtx_uid;
		goto fail_mtx;
	}

	memblock_uid = ksceKernelAllocMemBlock("RingBufferMemBlock", 0x6020D006, size, NULL);
	if (memblock_uid < 0) {
		ret = memblock_uid;
		goto fail_memblock;
	}
	ksceKernelGetMemBlockBase(memblock_uid, (void**)&base_ptr);

	buf_len = size;
	get_ptr = put_ptr = base_ptr;
	return 0;

fail_memblock:
	ksceKernelDeleteMutex(mtx_uid);
fail_mtx:
	ksceKernelDeleteEventFlag(evf_uid);
fail_evf:
	return ret;
}

int ringbuf_term(void) {
	ksceKernelDeleteEventFlag(evf_uid);
	ksceKernelDeleteMutex(mtx_uid);
	ksceKernelFreeMemBlock(memblock_uid);
	mtx_uid = memblock_uid = -1;
	buf_len = 0;
	base_ptr = get_ptr = put_ptr = NULL;
	return 0;
}

int ringbuf_put(char *c, int size) {
	int n_put = 0;
	ksceKernelLockMutex(mtx_uid, 1, NULL);

	while (size-- > 0) {
		if (put(*c++) == 0) {
			n_put++;
		} else {
			break;
		}
	}

	if (n_put > 0) {
		ksceKernelSetEventFlag(evf_uid, RINGBUF_EVF_NON_EMPTY);
	}

	ksceKernelUnlockMutex(mtx_uid, 1);
	return n_put;
}

int ringbuf_put_clobber(char *c, int size) {
	int n_put = 0;
	ksceKernelLockMutex(mtx_uid, 1, NULL);

	while (size-- > 0) {
		put_clobber(*c++);
		n_put++;
	}

	if (n_put > 0) {
		ksceKernelSetEventFlag(evf_uid, RINGBUF_EVF_NON_EMPTY);
	}

	ksceKernelUnlockMutex(mtx_uid, 1);
	return n_put;
}

int ringbuf_get(char *c, int size) {
	int n_get = 0;
	ksceKernelLockMutex(mtx_uid, 1, NULL);

	while (size-- > 0) {
		if (get(c++) == 0) {
			n_get++;
		} else {
			break;
		}
	}

	if (empty()) {
		ksceKernelClearEventFlag(evf_uid, ~RINGBUF_EVF_NON_EMPTY);
	}

	ksceKernelUnlockMutex(mtx_uid, 1);
	return n_get;
}

int ringbuf_get_wait(char *c, int size, SceUInt *timeout) {
	int n_get = 0;
	int ret = ksceKernelWaitEventFlag(evf_uid,
		RINGBUF_EVF_NON_EMPTY, SCE_KERNEL_EVF_WAITMODE_AND, NULL, timeout);
	if (ret < 0) {
		goto done;
	}
	ksceKernelLockMutex(mtx_uid, 1, NULL);

	while (size-- > 0) {
		if (get(c++) == 0) {
			n_get++;
		} else {
			break;
		}
	}

	if (empty()) {
		ksceKernelClearEventFlag(evf_uid, ~RINGBUF_EVF_NON_EMPTY);
	}

	ksceKernelUnlockMutex(mtx_uid, 1);
done:
	return n_get;
}
