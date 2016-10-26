#ifndef _CEDARDEV_API_H
#define _CEDARDEV_API_H

#include "config.h"

#if (CONFIG_MEMORY_DRIVER == OPTION_MEMORY_DRIVER_VE)
#include "cedardev_api_sun7i.h"
#elif (CONFIG_MEMORY_DRIVER == OPTION_MEMORY_DRIVER_ION)
#include "cedardev_api_sun8i.h"
#else
#error "invalid memory driver."
#endif

#endif /* _CEDARDEV_API_H */
