#ifndef _SUNXI_ALLOC_H
#define _SUNXI_ALLOC_H

int sunxi_alloc_open(void);
void sunxi_alloc_close(void);
void *sunxi_alloc_alloc(int size);
void sunxi_alloc_free(void *ptr);
void *sunxi_alloc_vir2phy(void *ptr);
void *sunxi_alloc_phy2vir(void *ptr);
void sunxi_flush_cache(void *startAddr, int size);

#endif /* _SUNXI_ALLOC_H */
