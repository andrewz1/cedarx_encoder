#ifndef _ION_ALLOC_H
#define _ION_ALLOC_H

int ion_alloc_open(void);
void ion_alloc_close(void);
void *ion_alloc_alloc(int size);
void ion_alloc_free(void *ptr);
void *ion_alloc_vir2phy(void *ptr);
void *ion_alloc_phy2vir(void *ptr);
void ion_flush_cache(void *startAddr, int size);

#endif /* _ION_ALLOC_H */
