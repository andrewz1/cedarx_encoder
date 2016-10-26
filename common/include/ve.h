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

#ifndef _VE_H
#define _VE_H

#include "ve_types.h"

int VeInitialize(void);
void VeRelease(void);
int VeLock(void);
void VeUnLock(void);
int VeEncoderLock(void);
void VeEncoderUnLock(void);
void VeSetDramType(void);
void VeReset(void);
int VeWaitInterrupt(void);
int VeWaitEncoderInterrupt(void);
void* VeGetRegisterBaseAddress(void);
unsigned int VeGetIcVersion(void);
int VeGetDramType(void);
int VeSetSpeed(int nSpeedMHz);
void VeEnableEncoder(void);
void VeDisableEncoder(void);
void VeEnableDecoder(enum VeRegionE region);
void VeDisableDecoder(void);
void VeDecoderWidthMode(int nWidth);
void VeResetDecoder(void);
void VeResetEncoder(void);
void VeInitEncoderPerformance(int nMode);
void VeUninitEncoderPerformance(int nMode);

#endif /* _VE_H */
