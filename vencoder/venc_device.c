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

#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>
#include <dirent.h>

#include "venc_device.h"
#include "list.h"
#include "export.h"

struct VEncoderNodeS {
	struct list_head lst;
	VENC_DEVICE *device;
	VENC_CODEC_TYPE type;
	char desc[64]; /* specific by mime type */
};

static LIST_HEAD(dev_lst);
static pthread_mutex_t dev_mx = PTHREAD_MUTEX_INITIALIZER;

EXPORT int VEncoderRegister(VENC_CODEC_TYPE type, char *desc, VENC_DEVICE *device) {
	struct VEncoderNodeS *d;

	if (!device)
		return -1;
	if (strlen(desc) > 63)
		return -1;
	pthread_mutex_lock(&dev_mx);
	/* check if conflict */
	list_for_each_entry(d, &dev_lst, lst)
		if (d->type == type) {
			pthread_mutex_unlock(&dev_mx);
			return -1;
		}
	d = calloc(1, sizeof(struct VEncoderNodeS));
	if (!d) {
		pthread_mutex_unlock(&dev_mx);
		return -1;
	}
	d->device = device;
	d->type = type;
	strncpy(d->desc, desc, 63);
	INIT_LIST_HEAD(&d->lst);
	list_add_tail(&d->lst, &dev_lst);
	pthread_mutex_unlock(&dev_mx);
	return 0;
}

EXPORT VENC_DEVICE *VencoderDeviceCreate(VENC_CODEC_TYPE type) {
	VENC_DEVICE *vencoder_device_handle = NULL;
	struct VEncoderNodeS *d;

	pthread_mutex_lock(&dev_mx);
	list_for_each_entry(d, &dev_lst, lst)
		if (d->type == type) {
			vencoder_device_handle = d->device;
			break;
		}
	pthread_mutex_unlock(&dev_mx);
	return vencoder_device_handle;
}

EXPORT void VencoderDeviceDestroy(void *handle) {
/* empty */
	return;
}

typedef void VEPluginEntry(void);

static void AddVEncPluginSingle(char *lib) {
	void *libFd;
	VEPluginEntry *PluginInit;

	libFd = dlopen(lib, RTLD_NOW);
	if (!libFd)
		return;
	PluginInit = dlsym(libFd, "CedarPluginVEncInit");
	if (!PluginInit)
		return;
	PluginInit(); /* init plugin */
}

#define LOCAL_LIB "libcedar_vencoder.so"

static int GetLocalPathFromProcessMaps(char *localPath, int len) {
	char path[512], line[1024];
	char *strLibPos, *rootPathPos;
	FILE *file;
	int localPathLen, rv = -1;

	memset(localPath, 0, len);
	sprintf(path, "/proc/%d/maps", getpid());
	file = fopen(path, "r");
	if (!file)
		return -1;
	while (fgets(line, 1023, file)) {
		strLibPos = strstr(line, LOCAL_LIB);
		if (!strLibPos)
			continue;
		rootPathPos = strchr(line, '/');
		if (!rootPathPos)
			break;
		localPathLen = strLibPos - rootPathPos - 1;
		if (localPathLen > len - 1)
			break;
		memcpy(localPath, rootPathPos, localPathLen);
		rv = 0;
		break;
	}
	fclose(file);
	return rv;
}

/* executive when load */
static void AddVEncPlugin(void) __attribute__((constructor));
void AddVEncPlugin(void) {
	struct dirent **namelist = NULL;
	char localPath[512];
	int num, i, ret;

//scan_local_path:
	ret = GetLocalPathFromProcessMaps(localPath, 512);
	if (ret)
		goto scan_system_lib;
	num = scandir(localPath, &namelist, NULL, NULL);
	if (num <= 0)
		goto scan_system_lib;
	for (i = 0; i < num; i++) {
		if (strstr(namelist[i]->d_name, "libcedar_plugin_venc") && strstr(namelist[i]->d_name, ".so"))
			AddVEncPluginSingle(namelist[i]->d_name);
		free(namelist[i]);
	}
scan_system_lib:
	// TODO: scan /system/lib
	return;
}
