#ifndef _VE_ALLOC_H
#define _VE_ALLOC_H

int ve_alloc_open(void);
void ve_alloc_close(void);
void *ve_alloc_alloc(int size);
void ve_alloc_free(void *ptr);
void *ve_alloc_vir2phy(void *ptr);
void *ve_alloc_phy2vir(void *ptr);
void ve_flush_cache(void *startAddr, int size);

#endif /* _VE_ALLOC_H */
