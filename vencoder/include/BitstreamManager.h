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

#ifndef _BITSTREAM_MANAGER_H
#define _BITSTREAM_MANAGER_H

#include <pthread.h>

typedef struct StreamInfo {
	int nStreamOffset;
	int nStreamLength;
	long long nPts;
	int nFlags;
	int nID;
} StreamInfo;

typedef struct BSListQ {
	StreamInfo *pStreamInfos;
	int nMaxFrameNum;
	int nValidFrameNum;
	int nUnReadFrameNum;
	int nReadPos;
	int nWritePos;
} BSListQ;

typedef struct BitStreamManager {
	pthread_mutex_t mutex;
	char *pStreamBuffer;
	char *pStreamBufferPhyAddrEnd;
	char *pStreamBufferPhyAddr;
	int nStreamBufferSize;
	int nWriteOffset;
	int nValidDataSize;
	BSListQ nBSListQ;
} BitStreamManager;

int BitStreamAddOneBitstream(BitStreamManager *handle, StreamInfo *pStreamInfo);
void *BitStreamBaseAddress(BitStreamManager *handle);
void *BitStreamBasePhyAddress(BitStreamManager *handle);
int BitStreamBufferSize(BitStreamManager *handle);
BitStreamManager *BitStreamCreate(int nBufferSize);
void BitStreamDestroy(BitStreamManager *handle);
void *BitStreamEndPhyAddress(BitStreamManager *handle);
int BitStreamFrameNum(BitStreamManager *handle);
int BitStreamFreeBufferSize(BitStreamManager *handle);
StreamInfo *BitStreamGetOneBitstream(BitStreamManager *handle);
int BitStreamReturnOneBitstream(BitStreamManager *handle, StreamInfo *pStreamInfo);
int BitStreamWriteOffset(BitStreamManager *handle);

#endif /* _BITSTREAM_MANAGER_H */
