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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ve.h"
#include "memoryAdapter.h"
#include "export.h"

EXPORT int EncAdapterInitialize(void) {
	if (VeInitialize() < 0)
		return -1;
	if (MemAdapterOpen() < 0) {
		VeRelease();
		return -1;
	}
	return 0;
}

EXPORT void EncAdpaterRelease(void) {
	MemAdapterClose();
	VeRelease();
}

EXPORT int EncAdapterLockVideoEngine(void) {
	return VeEncoderLock();
}

EXPORT void EncAdapterUnLockVideoEngine(void) {
	VeEncoderUnLock();
}

EXPORT void EncAdapterVeReset(void) {
	VeReset();
}

EXPORT int EncAdapterVeWaitInterrupt(void) {
	return VeWaitEncoderInterrupt();
}

EXPORT void *EncAdapterVeGetBaseAddress(void) {
	return VeGetRegisterBaseAddress();
}

EXPORT int EncAdapterMemGetDramType(void) {
	return VeGetDramType();
}

EXPORT void *EncAdapterMemPalloc(int nSize) {
	return MemAdapterPalloc(nSize);
}

EXPORT void EncAdapterMemPfree(void *pMem) {
	MemAdapterPfree(pMem);
}

EXPORT void EncAdapterMemFlushCache(void *pMem, int nSize) {
	MemAdapterFlushCache(pMem, nSize);
}

EXPORT void *EncAdapterMemGetPhysicAddress(void *pVirtualAddress) {
	return MemAdapterGetPhysicAddress(pVirtualAddress);
}

EXPORT void *EncAdapterMemGetVirtualAddress(void *pPhysicAddress) {
	return MemAdapterGetVirtualAddress(pPhysicAddress);
}

/* encoder functions */

EXPORT void EncAdapterEnableEncoder(void) {
	VeEnableEncoder();
}

EXPORT void EncAdapterDisableEncoder(void) {
	VeDisableEncoder();
}

EXPORT void EncAdapterResetEncoder(void) {
	VeResetEncoder();
}

EXPORT void EncAdapterInitPerformance(int nMode) {
	VeInitEncoderPerformance(nMode);
}

EXPORT void EncAdapterUninitPerformance(int nMode) {
	VeUninitEncoderPerformance(nMode);
}

EXPORT unsigned int EncAdapterGetICVersion(void) {
	return VeGetIcVersion();
}

EXPORT void EncAdapterSetDramType(void) {
	VeSetDramType();
}

/* debug functions */

EXPORT void EncAdapterPrintTopVEReg(void) {
	return;
}

EXPORT void EncAdapterPrintEncReg(void) {
	return;
}

EXPORT void EncAdapterPrintIspReg(void) {
	return;
}
