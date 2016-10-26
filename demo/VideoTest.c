#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vencoder.h"

int main(int argc, char **argv) {
	VencBaseConfig baseConfig;
	VencAllocateBufferParam bufferParam;
	VideoEncoder *pVideoEnc;
	VencInputBuffer inputBuffer;
	VencOutputBuffer outputBuffer;
	VencHeaderData sps_pps_data;
	VencH264Param h264Param;
	unsigned int src_width, src_height, dst_width, dst_height;
	unsigned int size_y, size_c, _size_y, _size_c;

	int i, testNumber = 226; /* frame num in file */
	FILE *in_file;
	FILE *out_file;

	testNumber = 250;
	src_width = dst_width = 1280;
	src_height = dst_height = 720;

	in_file = fopen("/dev/zero", "r");
	if (!in_file) {
		printf("open in_file fail\n");
		return 1;
	}
	out_file = fopen("/xxx.h264", "w");
	if (!out_file) {
		fclose(in_file);
		printf("open out_file fail\n");
		return -1;
	}

	memset(&h264Param, 0, sizeof(VencH264Param));
	h264Param.sProfileLevel.nProfile = VENC_H264ProfileMain;
	h264Param.sProfileLevel.nLevel = VENC_H264Level31;
	h264Param.bEntropyCodingCABAC = 1;
	h264Param.sQPRange.nMinqp = 10;
	h264Param.sQPRange.nMaxqp = 40;
	h264Param.nFramerate = 25; /* fps */
	h264Param.nBitrate = 4*1024*1024; /* bps */
	h264Param.nMaxKeyInterval = 25;
	h264Param.nCodingMode = VENC_FRAME_CODING;
//	h264Param.nCodingMode = VENC_FIELD_CODING;

	_size_y = src_width * src_height;
	_size_c = _size_y >> 1; /* div 2 */

	memset(&baseConfig, 0, sizeof(VencBaseConfig));
	baseConfig.nInputWidth = src_width;
	baseConfig.nInputHeight = src_height;
	baseConfig.nDstWidth = dst_width;
	baseConfig.nDstHeight = dst_height;
	baseConfig.nStride = src_width;
//	baseConfig.eInputFormat = VENC_PIXEL_YUV420P;
	baseConfig.eInputFormat = 0;

	memset(&bufferParam, 0, sizeof(VencAllocateBufferParam));
	bufferParam.nBufferNum = 4;
	bufferParam.nSizeY = _size_y;
	bufferParam.nSizeC = _size_c; /* 420P */

	pVideoEnc = VideoEncCreate(VENC_CODEC_H264);
	if (!pVideoEnc)
		goto err0;
	VideoEncSetParameter(pVideoEnc, VENC_IndexParamH264Param, &h264Param);
	VideoEncInit(pVideoEnc, &baseConfig);

//	VideoEncGetParameter(pVideoEnc, VENC_IndexParamH264SPSPPS, &sps_pps_data);
//	fwrite(sps_pps_data.pBuffer, 1, sps_pps_data.nLength, out_file);

	AllocInputBuffer(pVideoEnc, &bufferParam);

#if 0
typedef struct VencInputBuffer {
	unsigned long nID;
	long long nPts;
	unsigned int nFlag;
	unsigned char *pAddrPhyY;
	unsigned char *pAddrPhyC;
	unsigned char *pAddrVirY;
	unsigned char *pAddrVirC;
	int bEnableCorp;
	VencRect sCropInfo;
	int ispPicVar;
} VencInputBuffer;

typedef struct VencOutputBuffer {
	int nID;
	long long nPts;
	unsigned int nFlag;
	unsigned int nSize0;
	unsigned int nSize1;
	unsigned char *pData0;
	unsigned char *pData1;
} VencOutputBuffer;
#endif

	for (i = 0; i < testNumber; i++) {
		GetOneAllocInputBuffer(pVideoEnc, &inputBuffer);


		size_y = fread(inputBuffer.pAddrVirY, 1, _size_y, in_file);
		size_c = fread(inputBuffer.pAddrVirC, 1, _size_c, in_file);
		if (size_y != _size_y || size_c != _size_c) {
			fseek(in_file, 0L, SEEK_SET);
			size_y = fread(inputBuffer.pAddrVirY, 1, _size_y, in_file);
			size_c = fread(inputBuffer.pAddrVirC, 1, _size_c, in_file);
		}
		inputBuffer.bEnableCorp = 0;
		inputBuffer.nPts = i;

		printf("inp nID:%lu, nPts:%lld, nFlag:%u, ispPicVar:%d\n",
			inputBuffer.nID, inputBuffer.nPts, inputBuffer.nFlag, inputBuffer.ispPicVar);

		FlushCacheAllocInputBuffer(pVideoEnc, &inputBuffer);
		AddOneInputBuffer(pVideoEnc, &inputBuffer);

		VideoEncodeOneFrame(pVideoEnc);

		AlreadyUsedInputBuffer(pVideoEnc,&inputBuffer);
		ReturnOneAllocInputBuffer(pVideoEnc, &inputBuffer);

		printf("num %d\n", ValidBitstreamFrameNum(pVideoEnc));

		GetOneBitstreamFrame(pVideoEnc, &outputBuffer);

		printf("out nID:%d, nPts:%lld, nFlag:%u, nSize0:%u, nSize1:%u\n",
			outputBuffer.nID, outputBuffer.nPts, outputBuffer.nFlag, outputBuffer.nSize0, outputBuffer.nSize1);

		if (outputBuffer.nFlag & VENC_BUFFERFLAG_KEYFRAME) {
			VideoEncGetParameter(pVideoEnc, VENC_IndexParamH264SPSPPS, &sps_pps_data);
			fwrite(sps_pps_data.pBuffer, 1, sps_pps_data.nLength, out_file);
		}

		fwrite(outputBuffer.pData0, 1, outputBuffer.nSize0, out_file);
		if(outputBuffer.nSize1)
			fwrite(outputBuffer.pData1, 1, outputBuffer.nSize1, out_file);
		FreeOneBitStreamFrame(pVideoEnc, &outputBuffer);

		if(h264Param.nCodingMode == VENC_FIELD_CODING) {
			GetOneBitstreamFrame(pVideoEnc, &outputBuffer);
			fwrite(outputBuffer.pData0, 1, outputBuffer.nSize0, out_file);
			if(outputBuffer.nSize1)
				fwrite(outputBuffer.pData1, 1, outputBuffer.nSize1, out_file);
			FreeOneBitStreamFrame(pVideoEnc, &outputBuffer);
		}
	}







err0:
	fclose(out_file);
	fclose(in_file);
	return 0;
}
