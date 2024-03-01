/**
 * @file drv\inc\gpio_group.h
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
 * @author zhuodada
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
#define IOCTL_GROUP_GET					(0X0000)
#define IOCTL_GROUP_SET					IOCTL_GROUP_GET+1
#define IOCTL_GROUP_TOGGLE				IOCTL_GROUP_SET+1
#define IOCTL_GROUP_ENABLE				IOCTL_GROUP_TOGGLE+1
#define IOCTL_GROUP_SET_IRQ_HANDLER		IOCTL_GROUP_ENABLE+1
#define IOCTL_GROUP_POLL_CHECK			IOCTL_GROUP_SET_IRQ_HANDLER+1



#define GROUP_INDEX1	(1<<8)
#define GROUP_INDEX2	(2<<8)
#define GROUP_INDEX3	(3<<8)
#define GROUP_INDEX4	(4<<8)
#define GROUP_INDEX5	(5<<8)
#define GROUP_INDEX6	(6<<8)
#define GROUP_INDEX7	(7<<8)
#define GROUP_INDEX8	(8<<8)
#define GROUP_INDEX9	(9<<8)
#define GROUP_INDEX10	(10<<8)
#define GROUP_INDEX11	(11<<8)
#define GROUP_INDEX12	(12<<8)
#define GROUP_INDEX13	(13<<8)
#define GROUP_INDEX14	(14<<8)
#define GROUP_INDEX15	(15<<8)
#define GROUP_INDEX16	(16<<8)

/*---------- type define ----------*/
typedef enum 
{
	type_null_x_part = 0xF1,
	type_x_part1= 0xF2,
	type_x_part2= 0xF3,
	type_x_part3= 0xF4,
	type_x_part4= 0xF5,
	type_x_part5= 0xF6,
}all_part_type;
struct struct_all_type{
	all_part_type part1;
	all_part_type part2;
	all_part_type part3;
	all_part_type part4;
};

typedef struct {
    char *group_name;	
    struct {
        bool (*init)(void);
        void (*deinit)(void);
        unsigned char (*get)(unsigned char num);
        void (*set)(unsigned char num,unsigned char val);
		void (*user_function)(bool en);
    } ops;
	unsigned char group_num_max;	
	unsigned int  group_sta;
	void* user_date;
	void* user_date_special;
} gpio_group_describe_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/

#ifdef __cplusplus
}
#endif
#endif /* __DRIVER_GPIO_H */
