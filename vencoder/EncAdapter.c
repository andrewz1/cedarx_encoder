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

int EncAdapterInitialize(void) {
	if (VeInitialize() < 0)
		return -1;
	if (MemAdapterOpen() < 0) {
		VeRelease();
		return -1;
	}
	return 0;
}

void EncAdpaterRelease(void) {
	MemAdapterClose();
	VeRelease();
}

int EncAdapterLockVideoEngine(void) {
	return VeEncoderLock();
}

void EncAdapterUnLockVideoEngine(void) {
	VeEncoderUnLock();
}

void EncAdapterVeReset(void) {
	VeReset();
}

int EncAdapterVeWaitInterrupt(void) {
	return VeWaitEncoderInterrupt();
}

void *EncAdapterVeGetBaseAddress(void) {
	return VeGetRegisterBaseAddress();
}

int EncAdapterMemGetDramType(void) {
	return VeGetDramType();
}

void *EncAdapterMemPalloc(int nSize) {
	return MemAdapterPalloc(nSize);
}

void EncAdapterMemPfree(void *pMem) {
	MemAdapterPfree(pMem);
}

void EncAdapterMemFlushCache(void *pMem, int nSize) {
	MemAdapterFlushCache(pMem, nSize);
}

void *EncAdapterMemGetPhysicAddress(void *pVirtualAddress) {
	return MemAdapterGetPhysicAddress(pVirtualAddress);
}

void *EncAdapterMemGetVirtualAddress(void *pPhysicAddress) {
	return MemAdapterGetVirtualAddress(pPhysicAddress);
}

/* encoder functions */

void EncAdapterEnableEncoder(void) {
	VeEnableEncoder();
}

void EncAdapterDisableEncoder(void) {
	VeDisableEncoder();
}

void EncAdapterResetEncoder(void) {
	VeResetEncoder();
}

void EncAdapterInitPerformance(int nMode) {
	VeInitEncoderPerformance(nMode);
}

void EncAdapterUninitPerformance(int nMode) {
	VeUninitEncoderPerformance(nMode);
}

unsigned int EncAdapterGetICVersion(void) {
	return VeGetIcVersion();
}

void EncAdapterSetDramType(void) {
	VeSetDramType();
}

/* debug functions */

void EncAdapterPrintTopVEReg(void) {
	return;
}

void EncAdapterPrintEncReg(void) {
	return;
}


void EncAdapterPrintIspReg(void) {
	return;
}
