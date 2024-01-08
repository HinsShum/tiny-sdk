/**
 * @file drv\inc\gpio.h
 *
 * Copyright (C) 2021
 *
 * gpio.h is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author HinsShum hinsshum@qq.com
 *
 * @encoding utf-8
 */
#ifndef __DRIVER_GPIO_H
#define __DRIVER_GPIO_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "device.h"

/*---------- macro ----------*/
#define IOCTL_GROUP_GET                              (0X0000 + 0x06)
#define IOCTL_GROUP_SET                               IOCTL_GROUP_GET+1
#define IOCTL_GROUP_TOGGLE                            IOCTL_GROUP_SET+1
#define IOCTL_GROUP_SET_IRQ_HANDLER                   IOCTL_GROUP_TOGGLE+1


#define GROUP_INDEX1 								 0X0100
#define GROUP_INDEX2 								 0X0200
#define GROUP_INDEX3 								 0X0300
#define GROUP_INDEX4 								 0X0400
#define GROUP_INDEX5 								 0X0500
/*- ¡£¡£¡£ -*/
#define GROUP_INDEXA 								 0X2100// 33 =  1£¨all_index£©  +  32 £¨unsigned int = 32 bit£©

/*---------- type define ----------*/
typedef struct {
    struct {
        bool (*init)(void);
        void (*deinit)(void);
        unsigned char (*get)(unsigned char num);
        void (*set)(unsigned char num,bool val);
    } ops;
	unsigned int group_sta;
} gpio_group_describe_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/

#ifdef __cplusplus
}
#endif
#endif /* __DRIVER_GPIO_H */
