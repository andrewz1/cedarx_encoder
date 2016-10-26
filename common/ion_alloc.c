#include "config.h"

#if (CONFIG_MEMORY_DRIVER == OPTION_MEMORY_DRIVER_ION)

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

#include "ion.h"
#include "list.h"
#include "export.h"

#define PAGE_SIZE		4096
#define MEM_OFFSET		0x40000000UL

#define ION_IOC_SUNXI_FLUSH_RANGE		5
#define ION_IOC_SUNXI_FLUSH_ALL			6
#define ION_IOC_SUNXI_PHYS_ADDR			7
#define ION_IOC_SUNXI_DMA_COPY			8
#define ION_IOC_SUNXI_DUMP				9
#define ION_IOC_SUNXI_POOL_FREE			10

typedef struct {
	long start;
	long end;
} sunxi_cache_range;

typedef struct {
	void *handle;
	unsigned int phys_addr;
	unsigned int size;
} sunxi_phys_data;

struct ion_mem {
	void *virtAddr;
	void *physAddr;
	int size;
	int fd;
	struct ion_handle *handle;
	struct list_head lst;
};

static int ion_fd = -1;
static LIST_HEAD(im_lst);
static pthread_mutex_t ion_mx = PTHREAD_MUTEX_INITIALIZER;

int ion_alloc_open(void) {
	pthread_mutex_lock(&ion_mx);
	if (ion_fd >= 0) { /* opened */
		pthread_mutex_unlock(&ion_mx);
		return 0;
	}
	ion_fd = open("/dev/ion", O_RDONLY);
	if (ion_fd < 0) {
		ion_fd = -1;
		pthread_mutex_unlock(&ion_mx);
		return -1;
	}
	pthread_mutex_unlock(&ion_mx);
	return 0;
}

void ion_alloc_close(void) {
	pthread_mutex_lock(&ion_mx);
	if (ion_fd >= 0) {
		close(ion_fd);
		ion_fd = -1;
	}
	pthread_mutex_unlock(&ion_mx);
}

void *ion_alloc_alloc(int size) {
	int ret;
	struct ion_mem *im;
	struct ion_allocation_data alloc;
	struct ion_fd_data map;
	struct ion_custom_data custom;
	struct ion_handle_data handle;
	sunxi_phys_data phys;

	if (size <= 0)
		return NULL;
	pthread_mutex_lock(&ion_mx);
	if (ion_fd < 0) {
		pthread_mutex_unlock(&ion_mx);
		return NULL;
	}
	im = calloc(1, sizeof(struct ion_mem));
	if (!im) {
		pthread_mutex_unlock(&ion_mx);
		return NULL;
	}
	size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1); /* align to PAGE_SIZE */
	memset(&alloc, 0, sizeof(alloc));
	alloc.len = size;
	alloc.align = PAGE_SIZE;
	alloc.heap_id_mask = ION_HEAP_TYPE_DMA;
	alloc.flags = ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC;
	ret = ioctl(ion_fd, ION_IOC_ALLOC, &alloc);
	if (ret < 0) {
		free(im);
		pthread_mutex_unlock(&ion_mx);
		return NULL;
	}
	im->handle = alloc.handle;
	im->size = size;
	memset(&map, 0, sizeof(map));
	map.handle = alloc.handle;
	ret = ioctl(ion_fd, ION_IOC_MAP, &map);
	if (ret < 0) {
		handle.handle = alloc.handle;
		ioctl(ion_fd, ION_IOC_FREE, &handle);
		free(im);
		pthread_mutex_unlock(&ion_mx);
		return NULL;
	}
	im->fd = map.fd;
	im->virtAddr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, map.fd, 0);
	if (im->virtAddr == MAP_FAILED) {
		close(map.fd);
		handle.handle = alloc.handle;
		ioctl(ion_fd, ION_IOC_FREE, &handle);
		free(im);
		pthread_mutex_unlock(&ion_mx);
		return NULL;
	}
	memset(&phys, 0, sizeof(phys));
	phys.handle = alloc.handle;
	memset(&custom, 0, sizeof(custom));
	custom.cmd = ION_IOC_SUNXI_PHYS_ADDR;
	custom.arg = (unsigned long)(&phys);
	ret = ioctl(ion_fd, ION_IOC_CUSTOM, &custom);
	if (ret < 0) {
		munmap(im->virtAddr, size);
		close(map.fd);
		handle.handle = alloc.handle;
		ioctl(ion_fd, ION_IOC_FREE, &handle);
		free(im);
		pthread_mutex_unlock(&ion_mx);
		return NULL;
	}
	im->physAddr = (void *)phys.phys_addr - MEM_OFFSET;
	INIT_LIST_HEAD(&im->lst);
	list_add_tail(&im->lst, &im_lst);
	pthread_mutex_unlock(&ion_mx);
	return im->virtAddr;
}

void ion_alloc_free(void *ptr) {
	int found = 0;
	struct ion_mem *im;
	struct ion_handle_data handle;

	if (!ptr)
		return;
	pthread_mutex_lock(&ion_mx);
	if (ion_fd < 0) {
		pthread_mutex_unlock(&ion_mx);
		return;
	}
	list_for_each_entry(im, &im_lst, lst) {
		if (im->virtAddr == ptr) {
			found++;
			break;
		}
	}
	if (!found) {
		pthread_mutex_unlock(&ion_mx);
		return;
	}
	__list_del_entry(&im->lst);
	munmap(im->virtAddr, im->size);
	close(im->fd);
	handle.handle = im->handle;
	ioctl(ion_fd, ION_IOC_FREE, &handle);
	free(im);
	pthread_mutex_unlock(&ion_mx);
}

void *ion_alloc_vir2phy(void *ptr) {
	void *addr = NULL;
	struct ion_mem *im;

	if (!ptr)
		return NULL;
	pthread_mutex_lock(&ion_mx);
	if (ion_fd < 0) {
		pthread_mutex_unlock(&ion_mx);
		return NULL;
	}
	list_for_each_entry(im, &im_lst, lst) {
		if (im->virtAddr == NULL) {
			continue;
		} else if (im->virtAddr == ptr) {
			addr = im->physAddr;
			break;
		} else if (ptr > im->virtAddr && ptr < (im->virtAddr + im->size)) {
			addr = im->physAddr + (ptr - im->virtAddr);
			break;
		}
	}
	pthread_mutex_unlock(&ion_mx);
	return addr;
}

void *ion_alloc_phy2vir(void *ptr) {
	void *addr = NULL;
	struct ion_mem *im;

	if (!ptr)
		return NULL;
	pthread_mutex_lock(&ion_mx);
	if (ion_fd < 0) {
		pthread_mutex_unlock(&ion_mx);
		return NULL;
	}
	list_for_each_entry(im, &im_lst, lst) {
		if (im->physAddr == NULL) {
			continue;
		} else if (im->physAddr == ptr) {
			addr = im->virtAddr;
			break;
		} else if (ptr > im->physAddr && ptr < (im->physAddr + im->size)) {
			addr = im->virtAddr + (ptr - im->physAddr);
			break;
		}
	}
	pthread_mutex_unlock(&ion_mx);
	return addr;
}

void ion_flush_cache(void *startAddr, int size) {
	sunxi_cache_range range;
	struct ion_custom_data cache;

	if (ion_fd < 0)
		return;
	range.start = (long)startAddr;
	range.end = (long)(startAddr + size);
	cache.cmd = ION_IOC_SUNXI_FLUSH_RANGE;
	cache.arg = (unsigned long)(&range);
	ioctl(ion_fd, ION_IOC_CUSTOM, &cache);
}

EXPORT int MemAdapterOpen(void) __attribute__ ((alias ("ion_alloc_open")));
EXPORT void MemAdapterClose(void) __attribute__ ((alias ("ion_alloc_close")));
EXPORT void *MemAdapterPalloc(int nSize) __attribute__ ((alias ("ion_alloc_alloc")));
EXPORT void MemAdapterPfree(void *pMem) __attribute__ ((alias ("ion_alloc_free")));
EXPORT void *MemAdapterGetPhysicAddress(void *pVirtualAddress) __attribute__ ((alias ("ion_alloc_vir2phy")));
EXPORT void *MemAdapterGetVirtualAddress(void *pPhysicAddress) __attribute__ ((alias ("ion_alloc_phy2vir")));
EXPORT void MemAdapterFlushCache(void *pMem, int nSize) __attribute__ ((alias ("ion_flush_cache")));

#endif
