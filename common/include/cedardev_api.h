#ifndef _CEDARDEV_API_H
#define _CEDARDEV_API_H

#include "config.h"

#if ((CONFIG_MEMORY_DRIVER == OPTION_MEMORY_DRIVER_SUNXIMEM) || (CONFIG_MEMORY_DRIVER == OPTION_MEMORY_DRIVER_VE))
#include "cedardev_api_sun7i.h"
#elif ((CONFIG_MEMORY_DRIVER == OPTION_MEMORY_DRIVER_ION) || (CONFIG_MEMORY_DRIVER == OPTION_MEMORY_DRIVER_ION_LINUX_3_10))
#include "cedardev_api_sun8i.h"
#else
#error "invalid configuration of memory driver."
#endif

#endif /* _CEDARDEV_API_H */
