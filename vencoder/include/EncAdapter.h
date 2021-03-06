/*
 * Copyright (C) 2008-2015 Allwinner Technology Co. Ltd. 
 * Author: Ning Fang <fangning@allwinnertech.com>
 *         Caoyuan Yang <yangcaoyuan@allwinnertech.com>
 * 
 * This software is confidential and proprietary and may be used
 * only as expressly authorized by a licensing agreement from 
 * Softwinner Products. 
 *
 * The entire notice above must be reproduced on all copies 
 * and should not be removed. 
 */

#ifndef _ENC_ADAPTER_H
#define _ENC_ADAPTER_H

int EncAdapterInitialize(void);
void EncAdpaterRelease(void);
int EncAdapterLockVideoEngine(void);
void EncAdapterUnLockVideoEngine(void);
void EncAdapterVeReset(void);
int EncAdapterVeWaitInterrupt(void);
void *EncAdapterVeGetBaseAddress(void);
int EncAdapterMemGetDramType(void);
void *EncAdapterMemPalloc(int nSize);
void EncAdapterMemPfree(void* pMem);
void EncAdapterMemFlushCache(void* pMem, int nSize);
void *EncAdapterMemGetPhysicAddress(void* pVirtualAddress);
void *EncAdapterMemGetVirtualAddress(void* pPhysicAddress);
void EncAdapterEnableEncoder(void);
void EncAdapterDisableEncoder(void);
void EncAdapterResetEncoder(void);
void EncAdapterInitPerformance(int nMode);
void EncAdapterUninitPerformance(int nMode);
unsigned int EncAdapterGetICVersion(void);
void EncAdapterSetDramType(void);
void EncAdapterPrintTopVEReg(void);
void EncAdapterPrintEncReg(void);
void EncAdapterPrintIspReg(void);

#endif /* _ENC_ADAPTER_H */
