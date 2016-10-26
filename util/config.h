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

#ifndef _CDX_CONFIG_H
#define _CDX_CONFIG_H

// option for momory driver config.
#define OPTION_MEMORY_DRIVER_VE			1
#define OPTION_MEMORY_DRIVER_ION		2

// option for dram interface.
#define OPTION_DRAM_INTERFACE_DDR1_16BITS	1
#define OPTION_DRAM_INTERFACE_DDR1_32BITS	2
#define OPTION_DRAM_INTERFACE_DDR2_16BITS	3
#define OPTION_DRAM_INTERFACE_DDR2_32BITS	4
#define OPTION_DRAM_INTERFACE_DDR3_16BITS	5
#define OPTION_DRAM_INTERFACE_DDR3_32BITS	6
#define OPTION_DRAM_INTERFACE_DDR3_64BITS	7

// configuration.
#define CONFIG_MEMORY_DRIVER			OPTION_MEMORY_DRIVER_VE
#define CONFIG_DRAM_INTERFACE			OPTION_DRAM_INTERFACE_DDR3_32BITS

#endif /* _CDX_CONFIG_H */
