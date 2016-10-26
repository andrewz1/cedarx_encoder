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

#ifndef _MEMORY_ADAPTER_H
#define _MEMORY_ADAPTER_H

int MemAdapterOpen(void);
void MemAdapterClose(void);
void *MemAdapterPalloc(int nSize);
void MemAdapterPfree(void *pMem);
void MemAdapterFlushCache(void *pMem, int nSize);
void *MemAdapterGetPhysicAddress(void *pVirtualAddress);
void *MemAdapterGetVirtualAddress(void *pPhysicAddress);

#endif /* _MEMORY_ADAPTER_H */
