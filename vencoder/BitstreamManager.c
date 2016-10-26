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

#include "BitstreamManager.h"
#include "EncAdapter.h"

#define BITSTREAM_FRAME_FIFO_SIZE 256

#define ALIGN_32B(x) (((x) + (31)) & ~(31))
#define ALIGN_64B(x) (((x) + (63)) & ~(63))

BitStreamManager *BitStreamCreate(int nBufferSize) {
	BitStreamManager *handle;
	char *buffer;

	if (nBufferSize <= 0)
		return NULL;
	buffer = EncAdapterMemPalloc(nBufferSize);
	if (!buffer)
		return NULL;
	EncAdapterMemFlushCache(buffer, nBufferSize);
	handle = calloc(1, sizeof(BitStreamManager));
	if (!handle) {
		EncAdapterMemPfree(buffer);
		return NULL;
	}
	handle->nBSListQ.pStreamInfos = calloc(BITSTREAM_FRAME_FIFO_SIZE, sizeof(StreamInfo));
	if (!handle->nBSListQ.pStreamInfos) {
		free(handle);
		EncAdapterMemPfree(buffer);
		return NULL;
	}
	pthread_mutex_init(&handle->mutex, NULL);
	handle->pStreamBuffer = buffer;
	handle->pStreamBufferPhyAddr = EncAdapterMemGetPhysicAddress(handle->pStreamBuffer);
	handle->pStreamBufferPhyAddrEnd = handle->pStreamBufferPhyAddr + nBufferSize - 1;
	handle->nStreamBufferSize = nBufferSize;
	handle->nWriteOffset = 0;
	handle->nValidDataSize = 0;
	handle->nBSListQ.nMaxFrameNum = BITSTREAM_FRAME_FIFO_SIZE;
	handle->nBSListQ.nValidFrameNum = 0;
	handle->nBSListQ.nUnReadFrameNum = 0;
	handle->nBSListQ.nReadPos = 0;
	handle->nBSListQ.nWritePos = 0;
	return handle;
}

void BitStreamDestroy(BitStreamManager* handle) {
	if (handle) {
		pthread_mutex_destroy(&handle->mutex);
		if (handle->pStreamBuffer) {
			EncAdapterMemPfree(handle->pStreamBuffer);
			handle->pStreamBuffer = NULL;
		}
		if (handle->nBSListQ.pStreamInfos) {
			free(handle->nBSListQ.pStreamInfos);
			handle->nBSListQ.pStreamInfos = NULL;
		}
		free(handle);
	}
}

void *BitStreamBaseAddress(BitStreamManager* handle) {
	if (!handle)
		return NULL;
	return (void *)handle->pStreamBuffer;
}

void *BitStreamBasePhyAddress(BitStreamManager* handle) {
	if (!handle)
		return NULL;
	return (void *)handle->pStreamBufferPhyAddr;
}

void *BitStreamEndPhyAddress(BitStreamManager* handle) {
	if (!handle)
		return NULL;
	return (void *)handle->pStreamBufferPhyAddrEnd;
}

int BitStreamBufferSize(BitStreamManager* handle) {
	if (!handle)
		return 0;
	return handle->nStreamBufferSize;
}

int BitStreamFreeBufferSize(BitStreamManager* handle) {
	if (!handle)
		return 0;
	return (handle->nStreamBufferSize - handle->nValidDataSize);
}

int BitStreamFrameNum(BitStreamManager* handle) {
	if (!handle)
		return -1;
	return handle->nBSListQ.nValidFrameNum;
}

int BitStreamWriteOffset(BitStreamManager* handle) {
	if (!handle)
		return -1;
	return handle->nWriteOffset;
}

int BitStreamAddOneBitstream(BitStreamManager* handle, StreamInfo* pStreamInfo) {
	int nWritePos, NewWriteOffset;

	if (!handle || !pStreamInfo)
		return -1;
	if (pthread_mutex_lock(&handle->mutex))
		return -1;
	if (handle->nBSListQ.nValidFrameNum >= handle->nBSListQ.nMaxFrameNum) {
		pthread_mutex_unlock(&handle->mutex);
		return -1;
	}
	if (pStreamInfo->nStreamLength > (handle->nStreamBufferSize - handle->nValidDataSize)) {
		pthread_mutex_unlock(&handle->mutex);
		return -1;
	}
	nWritePos = handle->nBSListQ.nWritePos;
	memcpy(&handle->nBSListQ.pStreamInfos[nWritePos], pStreamInfo, sizeof(StreamInfo));
	handle->nBSListQ.pStreamInfos[nWritePos].nID = nWritePos;
	nWritePos++;
	if (nWritePos >= handle->nBSListQ.nMaxFrameNum)
		nWritePos = 0;
	handle->nBSListQ.nWritePos = nWritePos;
	handle->nBSListQ.nValidFrameNum++;
	handle->nBSListQ.nUnReadFrameNum++;
	handle->nValidDataSize += ALIGN_64B(pStreamInfo->nStreamLength); // encoder need 64 byte align
	NewWriteOffset = handle->nWriteOffset + ALIGN_64B(pStreamInfo->nStreamLength);
	if (NewWriteOffset >= handle->nStreamBufferSize)
		NewWriteOffset -= handle->nStreamBufferSize;
	handle->nWriteOffset = NewWriteOffset;
	pthread_mutex_unlock(&handle->mutex);
	return 0;
}

StreamInfo *BitStreamGetOneBitstream(BitStreamManager *handle) {
	StreamInfo *pStreamInfo;

	if (!handle)
		return NULL;
	if (pthread_mutex_lock(&handle->mutex))
		return NULL;
	if (handle->nBSListQ.nUnReadFrameNum == 0) {
		pthread_mutex_unlock(&handle->mutex);
		return NULL;
	}
	pStreamInfo = &handle->nBSListQ.pStreamInfos[handle->nBSListQ.nReadPos];
	if (!pStreamInfo) {
		pthread_mutex_unlock(&handle->mutex);
		return NULL;
	}
	handle->nBSListQ.nReadPos++;
	handle->nBSListQ.nUnReadFrameNum--;
	if (handle->nBSListQ.nReadPos >= handle->nBSListQ.nMaxFrameNum)
		handle->nBSListQ.nReadPos = 0;
	pthread_mutex_unlock(&handle->mutex);
	return pStreamInfo;
}

int BitStreamReturnOneBitstream(BitStreamManager* handle, StreamInfo* pStreamInfo) {
	int stream_size;

	if (!handle || !pStreamInfo)
		return -1;
	if (pStreamInfo->nID < 0 || pStreamInfo->nID > handle->nBSListQ.nMaxFrameNum)
		return -1;
	if (pthread_mutex_lock(&handle->mutex))
		return -1;
	if (handle->nBSListQ.nValidFrameNum == 0) {
		pthread_mutex_unlock(&handle->mutex);
		return -1;
	}
	stream_size = handle->nBSListQ.pStreamInfos[pStreamInfo->nID].nStreamLength;
	handle->nBSListQ.nValidFrameNum--;
	handle->nValidDataSize -= ALIGN_64B(stream_size);
	pthread_mutex_unlock(&handle->mutex);
	return 0;
}
