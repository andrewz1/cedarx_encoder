/*
 * Cedarx framework.
 * Copyright (c) 2008-2015 Allwinner Technology Co. Ltd.
 * Copyright (c) 2014 BZ Chen <bzchen@allwinnertech.com>
 *
 * This file is part of Cedarx.
 *
 * Cedarx is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 */

#include "config.h"

#if (CONFIG_MEMORY_DRIVER == OPTION_MEMORY_DRIVER_VE)

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "cedardev_api.h"
#include "list.h"
#include "export.h"

#define PAGE_SIZE		4096
#define PAGE_OFFSET		0xc0000000UL

struct ve_mem {
	void *virtAddr;
	void *physAddr;
	int size;
	struct list_head lst;
};

static int ve_fd = -1;
static LIST_HEAD(ve_lst);
static struct cedarv_env_infomation einfo;
static pthread_mutex_t ve_mx = PTHREAD_MUTEX_INITIALIZER;

int ve_alloc_open(void) {
	int ret;
	struct ve_mem *ve;

	pthread_mutex_lock(&ve_mx);
	if (ve_fd >= 0) { /* opened */
		pthread_mutex_unlock(&ve_mx);
		return 0;
	}
	ve_fd = open("/dev/cedar_dev", O_RDWR);
	if (ve_fd < 0) {
		ve_fd = -1;
		pthread_mutex_unlock(&ve_mx);
		return -1;
	}
	ret = ioctl(ve_fd, IOCTL_GET_ENV_INFO, (unsigned long)&einfo);
	if (ret < 0) {
		close(ve_fd);
		ve_fd = -1;
		pthread_mutex_unlock(&ve_mx);
		return -1;
	}
	if (!einfo.phymem_total_size) {
		memset(&einfo, 0, sizeof(einfo));
		close(ve_fd);
		ve_fd = -1;
		pthread_mutex_unlock(&ve_mx);
		return -1;
	}
	ve = calloc(1, sizeof(struct ve_mem));
	if (!ve) {
		memset(&einfo, 0, sizeof(einfo));
		close(ve_fd);
		ve_fd = -1;
		pthread_mutex_unlock(&ve_mx);
		return -1;
	}
	ve->physAddr = (void *)(einfo.phymem_start - PAGE_OFFSET);
	ve->size = einfo.phymem_total_size;
	INIT_LIST_HEAD(&ve->lst);
	list_add_tail(&ve->lst, &ve_lst);
	pthread_mutex_unlock(&ve_mx);
	return 0;
}

void ve_alloc_close(void) {
	struct ve_mem *ve, *tmp;

	pthread_mutex_lock(&ve_mx);
	if (ve_fd < 0) {
		pthread_mutex_unlock(&ve_mx);
		return;
	}
	memset(&einfo, 0, sizeof(einfo));
	close(ve_fd);
	ve_fd = -1;
	list_for_each_entry_safe(ve, tmp, &ve_lst, lst) {
		__list_del_entry(&ve->lst);
		if (ve->virtAddr)
			munmap(ve->virtAddr, ve->size);
		free(ve);
	}
	pthread_mutex_unlock(&ve_mx);
}

void *ve_alloc_alloc(int size) {
	struct ve_mem *ve, *best = NULL;
	void *addr;
	int leftSize;

	if (size <= 0)
		return NULL;
	pthread_mutex_lock(&ve_mx);
	if (ve_fd < 0) {
		pthread_mutex_unlock(&ve_mx);
		return NULL;
	}
	size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1); /* align to PAGE_SIZE */
	list_for_each_entry(ve, &ve_lst, lst) {
		if (ve->virtAddr) /* used slot */
			continue;
		if (ve->size < size) /* small slot */
			continue;
		if (!best || ve->size < best->size) /* pick smallest slot */
			best = ve;
		if (ve->size == size) /* stop search if size match */
			break;
	}
	if (!best) {
		pthread_mutex_unlock(&ve_mx);
		return NULL;
	}
	leftSize = best->size - size;
	addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, ve_fd, (off_t)(best->physAddr + PAGE_OFFSET));
	if (addr == MAP_FAILED) {
		pthread_mutex_unlock(&ve_mx);
		return NULL;
	}
	if (leftSize > 0) {
		ve = calloc(1, sizeof(struct ve_mem));
		if (!ve) {
			munmap(addr, size);
			pthread_mutex_unlock(&ve_mx);
			return NULL;
		}
		ve->physAddr = best->physAddr + size;
		ve->size = leftSize;
		INIT_LIST_HEAD(&ve->lst);
		list_add(&ve->lst, &best->lst);
	}
	best->virtAddr = addr;
	best->size = size;
	pthread_mutex_unlock(&ve_mx);
	return addr;
}

void ve_alloc_free(void *ptr) {
	struct ve_mem *ve, *ne, *tmp, *tmp2;
	int found = 0;

	if (!ptr)
		return;
	pthread_mutex_lock(&ve_mx);
	if (ve_fd < 0) {
		pthread_mutex_unlock(&ve_mx);
		return;
	}
	list_for_each_entry(ve, &ve_lst, lst) {
		if (ve->virtAddr == ptr) {
			munmap(ve->virtAddr, ve->size);
			ve->virtAddr = NULL;
			found++;
			break;
		}
	}
	if (!found) {
		pthread_mutex_unlock(&ve_mx);
		return;
	}
	list_for_each_entry_safe(ve, tmp, &ve_lst, lst) {
		if (ve->virtAddr)
			continue;
		ne = ve;
		list_for_each_entry_safe_continue(ne, tmp2, &ve_lst, lst) {
			if (ne->virtAddr)
				break;
			__list_del_entry(&ne->lst);
			ve->size += ne->size;
			free(ne);
		}
	}
	pthread_mutex_unlock(&ve_mx);
}

void *ve_alloc_vir2phy(void *ptr) {
	struct ve_mem *ve;
	void *addr = NULL;

	if (!ptr)
		return NULL;
	pthread_mutex_lock(&ve_mx);
	if (ve_fd < 0) {
		pthread_mutex_unlock(&ve_mx);
		return NULL;
	}
	list_for_each_entry(ve, &ve_lst, lst) {
		if (!ve->virtAddr)
			continue;
		if (ve->virtAddr == ptr) {
			addr = ve->physAddr;
			break;
		} else if (ptr > ve->virtAddr && ptr < (ve->virtAddr + ve->size)) {
			addr = ve->physAddr + (ptr - ve->virtAddr);
			break;
		}
	}
	pthread_mutex_unlock(&ve_mx);
	return addr;
}

void *ve_alloc_phy2vir(void *ptr) {
	struct ve_mem *ve;
	void *addr = NULL;

	if (!ptr)
		return NULL;
	pthread_mutex_lock(&ve_mx);
	if (ve_fd < 0) {
		pthread_mutex_unlock(&ve_mx);
		return NULL;
	}
	list_for_each_entry(ve, &ve_lst, lst) {
		if (!ve->physAddr)
			continue;
		if (ve->physAddr == ptr) {
			addr = ve->virtAddr;
			break;
		} else if (ptr > ve->physAddr && ptr < (ve->physAddr + ve->size)) {
			addr = ve->virtAddr + (ptr - ve->physAddr);
			break;
		}
	}
	pthread_mutex_unlock(&ve_mx);
	return addr;
}

void ve_flush_cache(void *startAddr, int size) {
	struct cedarv_cache_range range;

	if (ve_fd < 0)
		return;
	range.start = (long)startAddr;
	range.end = (long)(startAddr + size);
	ioctl(ve_fd, IOCTL_FLUSH_CACHE, &range);
}

EXPORT int MemAdapterOpen(void) __attribute__ ((alias ("ve_alloc_open")));
EXPORT void MemAdapterClose(void) __attribute__ ((alias ("ve_alloc_close")));
EXPORT void *MemAdapterPalloc(int nSize) __attribute__ ((alias ("ve_alloc_alloc")));
EXPORT void MemAdapterPfree(void *pMem) __attribute__ ((alias ("ve_alloc_free")));
EXPORT void *MemAdapterGetPhysicAddress(void *pVirtualAddress) __attribute__ ((alias ("ve_alloc_vir2phy")));
EXPORT void *MemAdapterGetVirtualAddress(void *pPhysicAddress) __attribute__ ((alias ("ve_alloc_phy2vir")));
EXPORT void MemAdapterFlushCache(void *pMem, int nSize) __attribute__ ((alias ("ve_flush_cache")));

#endif
