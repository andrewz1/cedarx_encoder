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

#include "FrameBufferManager.h"
#include "EncAdapter.h"
#include "venc_device.h"

#define FRAME_BUFFER_NUM 4

typedef struct VencContext {
	VENC_DEVICE* pVEncDevice;
	void* pEncoderHandle;
	FrameBufferManager* pFBM;
	VencBaseConfig baseConfig;
	unsigned int nFrameBufferNum;
	VencHeaderData headerData;
	VencInputBuffer curEncInputbuffer;
	VENC_CODEC_TYPE codecType;
	unsigned int ICVersion;
	int bInit;
} VencContext;

VideoEncoder* VideoEncCreate(VENC_CODEC_TYPE eCodecType) {
	VencContext* venc_ctx;

	if (EncAdapterInitialize())
		return NULL;
	venc_ctx = calloc(1, sizeof(VencContext));
	if (!venc_ctx) {
		EncAdpaterRelease();
		return NULL;
	}
	venc_ctx->nFrameBufferNum = FRAME_BUFFER_NUM;
	venc_ctx->codecType = eCodecType;
	venc_ctx->ICVersion = EncAdapterGetICVersion();
	venc_ctx->bInit = 0;
	venc_ctx->pVEncDevice = VencoderDeviceCreate(eCodecType);
	if (!venc_ctx->pVEncDevice) {
		free(venc_ctx);
		EncAdpaterRelease();
		return NULL;
	}
	venc_ctx->pEncoderHandle = venc_ctx->pVEncDevice->open();
	if (!venc_ctx->pEncoderHandle) {
		VencoderDeviceDestroy(venc_ctx->pVEncDevice);
		free(venc_ctx);
		EncAdpaterRelease();
		return NULL;
	}
	return (VideoEncoder*)venc_ctx;
}

void VideoEncDestroy(VideoEncoder *pEncoder) {
	VencContext *venc_ctx = (VencContext *)pEncoder;

	if (!pEncoder)
		return;
	VideoEncUnInit(pEncoder);
	if (venc_ctx->pVEncDevice) {
		venc_ctx->pVEncDevice->close(venc_ctx->pEncoderHandle);
		VencoderDeviceDestroy(venc_ctx->pVEncDevice);
		venc_ctx->pEncoderHandle = NULL;
		venc_ctx->pVEncDevice = NULL;
	}
	EncAdpaterRelease();
	free(venc_ctx);
}

int VideoEncInit(VideoEncoder *pEncoder, VencBaseConfig *pConfig) {
	VencContext *venc_ctx = (VencContext *)pEncoder;
	int result = 0;

	if (!pEncoder || !pConfig || venc_ctx->bInit)
		return -1;
	venc_ctx->pFBM = FrameBufferManagerCreate(venc_ctx->nFrameBufferNum);
	if (!venc_ctx->pFBM)
		return -1;
	if (venc_ctx->ICVersion == 0x1639) {
		if (pConfig->nDstWidth >= 3840 || pConfig->nDstHeight >= 2160)
			EncAdapterInitPerformance(1);
		else
			EncAdapterInitPerformance(0);
	}
	memcpy(&venc_ctx->baseConfig, pConfig, sizeof(VencBaseConfig));
	EncAdapterLockVideoEngine();
	result = venc_ctx->pVEncDevice->init(venc_ctx->pEncoderHandle, &venc_ctx->baseConfig);
	EncAdapterUnLockVideoEngine();
	venc_ctx->bInit = 1;
	return result;
}

int VideoEncUnInit(VideoEncoder *pEncoder) {
	VencContext *venc_ctx = (VencContext *)pEncoder;

	if (!pEncoder || !venc_ctx->bInit)
		return -1;
	venc_ctx->pVEncDevice->uninit(venc_ctx->pEncoderHandle);
	if (venc_ctx->ICVersion == 0x1639) {
		if (venc_ctx->baseConfig.nDstWidth >= 3840 || venc_ctx->baseConfig.nDstHeight >= 2160)
			EncAdapterUninitPerformance(1);
		else
			EncAdapterUninitPerformance(0);
	}
	if (venc_ctx->pFBM) {
		FrameBufferManagerDestroy(venc_ctx->pFBM);
		venc_ctx->pFBM = NULL;
	}
	venc_ctx->bInit = 0;
	return 0;
}

int AllocInputBuffer(VideoEncoder *pEncoder, VencAllocateBufferParam *pBufferParam) {
	VencContext *venc_ctx = (VencContext *)pEncoder;

	if (!pEncoder || !pBufferParam || !venc_ctx->pFBM)
		return -1;
	return AllocateInputBuffer(venc_ctx->pFBM, pBufferParam);
}

int GetOneAllocInputBuffer(VideoEncoder* pEncoder, VencInputBuffer* pInputbuffer) {
	VencContext *venc_ctx = (VencContext *)pEncoder;

	if (!pEncoder || !pInputbuffer || !venc_ctx->pFBM)
		return -1;
	return GetOneAllocateInputBuffer(venc_ctx->pFBM, pInputbuffer);
}

int FlushCacheAllocInputBuffer(VideoEncoder* pEncoder, VencInputBuffer *pInputbuffer) {
	VencContext *venc_ctx = (VencContext *)pEncoder;

	if (!pEncoder || !pInputbuffer || !venc_ctx->pFBM)
		return -1;
	FlushCacheAllocateInputBuffer(venc_ctx->pFBM, pInputbuffer);
	return 0;
}

int ReturnOneAllocInputBuffer(VideoEncoder* pEncoder, VencInputBuffer *pInputbuffer) {
	VencContext* venc_ctx = (VencContext*) pEncoder;

	if (!pEncoder || !pInputbuffer || !venc_ctx->pFBM)
		return -1;
	return ReturnOneAllocateInputBuffer(venc_ctx->pFBM, pInputbuffer);
}

int ReleaseAllocInputBuffer(VideoEncoder* pEncoder) {
	if (!pEncoder)
		return -1;
	return 0;
}

int AddOneInputBuffer(VideoEncoder* pEncoder, VencInputBuffer* pBuffer) {
	VencContext* venc_ctx = (VencContext*) pEncoder;

	if (!pEncoder || !pBuffer || !venc_ctx->pFBM)
		return -1;
	return AddInputBuffer(venc_ctx->pFBM, pBuffer);
}

int VideoEncodeOneFrame(VideoEncoder* pEncoder) {
	VencContext* venc_ctx = (VencContext*)pEncoder;
	unsigned long offt;
	int result;

	if (!pEncoder || !venc_ctx->pFBM)
		return -1;
	if (GetInputBuffer(venc_ctx->pFBM, &venc_ctx->curEncInputbuffer))
		return VENC_RESULT_NO_FRAME_BUFFER;
	if (venc_ctx->ICVersion == 0x1639)
		offt = 0x20000000;
	else
		offt = 0x40000000;
	if ((unsigned long)(venc_ctx->curEncInputbuffer.pAddrPhyY) >= offt)
		venc_ctx->curEncInputbuffer.pAddrPhyY -= offt;
	if ((unsigned long)(venc_ctx->curEncInputbuffer.pAddrPhyC) >= offt)
		venc_ctx->curEncInputbuffer.pAddrPhyC -= offt;
	EncAdapterLockVideoEngine();
	result = venc_ctx->pVEncDevice->encode(venc_ctx->pEncoderHandle, &venc_ctx->curEncInputbuffer);
	EncAdapterUnLockVideoEngine();
	AddUsedInputBuffer(venc_ctx->pFBM, &venc_ctx->curEncInputbuffer);
	return result;
}

int AlreadyUsedInputBuffer(VideoEncoder *pEncoder, VencInputBuffer *pBuffer) {
	VencContext *venc_ctx = (VencContext *)pEncoder;

	if (!pEncoder || !pBuffer || !venc_ctx->pFBM)
		return -1;
	return GetUsedInputBuffer(venc_ctx->pFBM, pBuffer);
}

int ValidBitstreamFrameNum(VideoEncoder *pEncoder) {
	VencContext *venc_ctx = (VencContext *)pEncoder;

	if (!pEncoder)
		return -1;
	return venc_ctx->pVEncDevice->ValidBitStreamFrameNum(venc_ctx->pEncoderHandle);
}

int GetOneBitstreamFrame(VideoEncoder *pEncoder, VencOutputBuffer *pBuffer) {
	VencContext *venc_ctx = (VencContext *)pEncoder;

	if (!pEncoder || !pBuffer)
		return -1;
	return venc_ctx->pVEncDevice->GetOneBitStreamFrame(venc_ctx->pEncoderHandle, pBuffer);
}

int FreeOneBitStreamFrame(VideoEncoder *pEncoder, VencOutputBuffer *pBuffer) {
	VencContext *venc_ctx = (VencContext *)pEncoder;

	if (!pEncoder || !pBuffer)
		return -1;
	return venc_ctx->pVEncDevice->FreeOneBitStreamFrame(venc_ctx->pEncoderHandle, pBuffer);
}

int VideoEncGetParameter(VideoEncoder *pEncoder, VENC_INDEXTYPE indexType, void *paramData) {
	VencContext *venc_ctx = (VencContext *)pEncoder;

	if (!pEncoder)
		return -1;
	return venc_ctx->pVEncDevice->GetParameter(venc_ctx->pEncoderHandle, indexType, paramData);
}

int VideoEncSetParameter(VideoEncoder *pEncoder, VENC_INDEXTYPE indexType, void *paramData) {
	VencContext *venc_ctx = (VencContext *)pEncoder;

	if (!pEncoder)
		return -1;
	return venc_ctx->pVEncDevice->SetParameter(venc_ctx->pEncoderHandle, indexType, paramData);
}

int AWJpecEnc(JpegEncInfo *pJpegInfo, EXIFInfo *pExifInfo, void *pOutBuffer, int *pOutBufferSize) {
	VencAllocateBufferParam bufferParam;
	VideoEncoder *pVideoEnc;
	VencInputBuffer inputBuffer;
	VencOutputBuffer outputBuffer;
	int result = -1;

	pVideoEnc = VideoEncCreate(VENC_CODEC_JPEG);
	VideoEncSetParameter(pVideoEnc, VENC_IndexParamJpegExifInfo, pExifInfo);
	VideoEncSetParameter(pVideoEnc, VENC_IndexParamJpegQuality, &pJpegInfo->quality);
	if (VideoEncInit(pVideoEnc, &pJpegInfo->sBaseInfo) < 0)
		goto err0;
	if (pJpegInfo->bNoUseAddrPhy) {
		bufferParam.nSizeY = pJpegInfo->sBaseInfo.nStride * pJpegInfo->sBaseInfo.nInputHeight;
		bufferParam.nSizeC = bufferParam.nSizeY >> 1;
		bufferParam.nBufferNum = 1;
		if (AllocInputBuffer(pVideoEnc, &bufferParam) < 0)
			goto err0;
		GetOneAllocInputBuffer(pVideoEnc, &inputBuffer);
		memcpy(inputBuffer.pAddrVirY, pJpegInfo->pAddrPhyY, bufferParam.nSizeY);
		memcpy(inputBuffer.pAddrVirC, pJpegInfo->pAddrPhyC, bufferParam.nSizeC);
		FlushCacheAllocInputBuffer(pVideoEnc, &inputBuffer);
	} else {
		inputBuffer.pAddrPhyY = pJpegInfo->pAddrPhyY;
		inputBuffer.pAddrPhyC = pJpegInfo->pAddrPhyC;
	}
	inputBuffer.bEnableCorp = pJpegInfo->bEnableCorp;
	inputBuffer.sCropInfo.nLeft = pJpegInfo->sCropInfo.nLeft;
	inputBuffer.sCropInfo.nTop = pJpegInfo->sCropInfo.nTop;
	inputBuffer.sCropInfo.nWidth = pJpegInfo->sCropInfo.nWidth;
	inputBuffer.sCropInfo.nHeight = pJpegInfo->sCropInfo.nHeight;
	AddOneInputBuffer(pVideoEnc, &inputBuffer);
//	if (VideoEncodeOneFrame(pVideoEnc) != 0) {
//		loge("jpeg encoder error");
//	}
	AlreadyUsedInputBuffer(pVideoEnc, &inputBuffer);
	if (pJpegInfo->bNoUseAddrPhy)
		ReturnOneAllocInputBuffer(pVideoEnc, &inputBuffer);
	GetOneBitstreamFrame(pVideoEnc, &outputBuffer);
	memcpy(pOutBuffer, outputBuffer.pData0, outputBuffer.nSize0);
	if (outputBuffer.nSize1) {
		memcpy(((unsigned char*) pOutBuffer + outputBuffer.nSize0), outputBuffer.pData1, outputBuffer.nSize1);
		*pOutBufferSize = outputBuffer.nSize0 + outputBuffer.nSize1;
	} else {
		*pOutBufferSize = outputBuffer.nSize0;
	}
	FreeOneBitStreamFrame(pVideoEnc, &outputBuffer);
	result = 0;
err0:
	if (pVideoEnc)
		VideoEncDestroy(pVideoEnc);
	return result;
}
