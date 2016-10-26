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

#include "config.h"

#include "cedardev_api.h"
#include "ve_types.h"
#include "export.h"

#define VE_MODE_SELECT		0x00
#define VE_RESET			0x04

static int gVeDriverFd = -1;
static int gVeRefCount = 0;
static struct cedarv_env_infomation gVeEnvironmentInfo;

static pthread_mutex_t gVeMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gVeRegisterMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t gVeDecoderMutex = PTHREAD_MUTEX_INITIALIZER;

void VeReset(void);
int VeGetDramType(void);

EXPORT int VeInitialize(void) {
	int ret, rv = -1;

	pthread_mutex_lock(&gVeMutex);
	if (gVeRefCount < 0)
		goto err0;
	if (gVeRefCount == 0) {
		gVeDriverFd = open("/dev/cedar_dev", O_RDWR);
		if (gVeDriverFd < 0) {
			gVeDriverFd = -1;
			goto err0;
		}
		ret = ioctl(gVeDriverFd, IOCTL_SET_REFCOUNT, 0);
		if (ret < 0) {
			close(gVeDriverFd);
			gVeDriverFd = -1;
			goto err0;
		}
		//* request ve.
		ret = ioctl(gVeDriverFd, IOCTL_ENGINE_REQ, 0);
		if (ret < 0) {
			close(gVeDriverFd);
			gVeDriverFd = -1;
			goto err0;
		}
		//* map registers.
		ret = ioctl(gVeDriverFd, IOCTL_GET_ENV_INFO, (unsigned long)&gVeEnvironmentInfo);
		if (ret < 0) {
			close(gVeDriverFd);
			gVeDriverFd = -1;
			goto err0;
		}
		gVeEnvironmentInfo.address_macc = (unsigned int)mmap(NULL, 2048, PROT_READ | PROT_WRITE, MAP_SHARED, gVeDriverFd, (off_t)gVeEnvironmentInfo.address_macc);
		//* reset ve.
		VeReset();
	}
	gVeRefCount++;
	rv = 0;
err0:
	pthread_mutex_unlock(&gVeMutex);
	return rv;
}

EXPORT void VeRelease(void) {
	pthread_mutex_lock(&gVeMutex);
	if (gVeRefCount <= 0)
		goto err0;
	gVeRefCount--;
	if (gVeRefCount == 0) {
		if (gVeDriverFd != -1) {
			ioctl(gVeDriverFd, IOCTL_ENGINE_REL, 0);
			munmap((void *)gVeEnvironmentInfo.address_macc, 2048);
			close(gVeDriverFd);
			gVeDriverFd = -1;
		}
	}
err0:
	pthread_mutex_unlock(&gVeMutex);
	return;
}

EXPORT int VeLock(void) {
	return pthread_mutex_lock(&gVeDecoderMutex);
}

EXPORT void VeUnLock(void) {
	pthread_mutex_unlock(&gVeDecoderMutex);
}

EXPORT int VeEncoderLock(void) __attribute__ ((alias("VeLock")));
EXPORT void VeEncoderUnLock(void) __attribute__ ((alias("VeUnLock")));

EXPORT void VeSetDramType(void) {
	volatile vetop_reg_mode_sel_t* pVeModeSelect;

	pthread_mutex_lock(&gVeRegisterMutex);
	pVeModeSelect = (vetop_reg_mode_sel_t*)(gVeEnvironmentInfo.address_macc + VE_MODE_SELECT);
	switch (VeGetDramType()) {
	case DDRTYPE_DDR1_16BITS:
		pVeModeSelect->ddr_mode = 0;
		break;
	case DDRTYPE_DDR1_32BITS:
	case DDRTYPE_DDR2_16BITS:
		pVeModeSelect->ddr_mode = 1;
		break;
	case DDRTYPE_DDR2_32BITS:
	case DDRTYPE_DDR3_16BITS:
		pVeModeSelect->ddr_mode = 2;
		break;
	case DDRTYPE_DDR3_32BITS:
	case DDRTYPE_DDR3_64BITS:
		pVeModeSelect->ddr_mode = 3;
		pVeModeSelect->rec_wr_mode = 1;
		break;
	default:
		break;
	}
	pthread_mutex_unlock(&gVeRegisterMutex);
}

EXPORT void VeReset(void) {
	ioctl(gVeDriverFd, IOCTL_RESET_VE, 0);
	VeSetDramType();
}

EXPORT int VeWaitInterrupt(void) {
	if (ioctl(gVeDriverFd, IOCTL_WAIT_VE_DE, 1) <= 0)
		return -1;  //* wait ve interrupt fail.
	return 0;
}

EXPORT int VeWaitEncoderInterrupt(void) {
	if (ioctl(gVeDriverFd, IOCTL_WAIT_VE_EN, 1) <= 0)
		return -1;  //* wait ve interrupt fail.
	return 0;
}

EXPORT void *VeGetRegisterBaseAddress(void) {
	return (void*)gVeEnvironmentInfo.address_macc;
}

EXPORT unsigned int VeGetIcVersion(void) {
	volatile unsigned int value;

	if (gVeRefCount > 0) {
		value = *((unsigned int*)((char *)gVeEnvironmentInfo.address_macc + 0xf0));
		return (value >> 16);
	}
	return 0;
}

EXPORT int VeGetDramType(void) {
	//* can we know memory type by some system api?
#if CONFIG_DRAM_INTERFACE == OPTION_DRAM_INTERFACE_DDR1_16BITS
	return DDRTYPE_DDR1_16BITS;
#elif CONFIG_DRAM_INTERFACE == OPTION_DRAM_INTERFACE_DDR1_32BITS
	return DDRTYPE_DDR1_32BITS;
#elif CONFIG_DRAM_INTERFACE == OPTION_DRAM_INTERFACE_DDR2_16BITS
	return DDRTYPE_DDR2_16BITS;
#elif CONFIG_DRAM_INTERFACE == OPTION_DRAM_INTERFACE_DDR2_32BITS
	return DDRTYPE_DDR2_32BITS;
#elif CONFIG_DRAM_INTERFACE == OPTION_DRAM_INTERFACE_DDR3_16BITS
	return DDRTYPE_DDR3_16BITS;
#elif CONFIG_DRAM_INTERFACE == OPTION_DRAM_INTERFACE_DDR3_32BITS
	return DDRTYPE_DDR3_32BITS;
#elif CONFIG_DRAM_INTERFACE == OPTION_DRAM_INTERFACE_DDR3_64BITS
	return DDRTYPE_DDR3_64BITS;
#else
#error "invalid dram type."
#endif
}

EXPORT int VeSetSpeed(int nSpeedMHz) {
	return ioctl(gVeDriverFd, IOCTL_SET_VE_FREQ, nSpeedMHz);
}

EXPORT void VeEnableEncoder(void) {
	volatile vetop_reg_mode_sel_t* pVeModeSelect;

	pthread_mutex_lock(&gVeRegisterMutex);
	pVeModeSelect = (vetop_reg_mode_sel_t*)(gVeEnvironmentInfo.address_macc + VE_MODE_SELECT);
	pVeModeSelect->mode = 11;
	pVeModeSelect->enc_enable = 1;
	pVeModeSelect->enc_isp_enable = 1;
	pthread_mutex_unlock(&gVeRegisterMutex);
}

EXPORT void VeDisableEncoder(void) {
	volatile vetop_reg_mode_sel_t* pVeModeSelect;

	pthread_mutex_lock(&gVeRegisterMutex);
	pVeModeSelect = (vetop_reg_mode_sel_t*) (gVeEnvironmentInfo.address_macc + VE_MODE_SELECT);
	pVeModeSelect->mode = 0x7;
	pVeModeSelect->enc_enable = 0;
	pVeModeSelect->enc_isp_enable = 0;
	pthread_mutex_unlock(&gVeRegisterMutex);
}

EXPORT void VeEnableDecoder(enum VeRegionE region) {
	volatile vetop_reg_mode_sel_t* pVeModeSelect;

	pthread_mutex_lock(&gVeRegisterMutex);
	pVeModeSelect = (vetop_reg_mode_sel_t*) (gVeEnvironmentInfo.address_macc + VE_MODE_SELECT);
//	pVeModeSelect->mode = nDecoderMode;
	switch (region) {
	case VE_REGION_0:
		pVeModeSelect->mode = 0;
		break;
	case VE_REGION_1:
		pVeModeSelect->mode = 1; //* MPEG1/2/4 or JPEG decoder.
		break;
	case VE_REGION_2:
	case VE_REGION_3:
	default:
		pVeModeSelect->mode = 0; //* MPEG1/2/4 or JPEG decoder.
		break;
	}
	pthread_mutex_unlock(&gVeRegisterMutex);
}

EXPORT void VeDisableDecoder(void) {
	volatile vetop_reg_mode_sel_t* pVeModeSelect;

	pthread_mutex_lock(&gVeRegisterMutex);
	pVeModeSelect = (vetop_reg_mode_sel_t*) (gVeEnvironmentInfo.address_macc + VE_MODE_SELECT);
	pVeModeSelect->mode = 7;
	pthread_mutex_unlock(&gVeRegisterMutex);
}

EXPORT void VeDecoderWidthMode(int nWidth) {
	volatile vetop_reg_mode_sel_t* pVeModeSelect;

	pthread_mutex_lock(&gVeRegisterMutex);
	pVeModeSelect = (vetop_reg_mode_sel_t*)(gVeEnvironmentInfo.address_macc + VE_MODE_SELECT);
	if (nWidth >= 2048)
		pVeModeSelect->pic_width_more_2048 = 1;
	else
		pVeModeSelect->pic_width_more_2048 = 0;
	pthread_mutex_unlock(&gVeRegisterMutex);
}

EXPORT void VeResetDecoder(void) __attribute__ ((alias("VeReset")));
EXPORT void VeResetEncoder(void) __attribute__ ((alias("VeReset")));

EXPORT void VeInitEncoderPerformance(int nMode) { //* 0: normal performance; 1. high performance
	return;
}

EXPORT void VeUninitEncoderPerformance(int nMode) { //* 0: normal performance; 1. high performance
	return;
}
