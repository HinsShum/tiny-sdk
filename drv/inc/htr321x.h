/**
 * @file drv\inc\htr321x.h
 *
 * Copyright (C) 2023
 *
 * bsp_key1.h is free software: you can redistribute it and/or modify
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
#ifndef __HTR321X_H
#define __HTR321X_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include "device.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

    /*---------- macro ----------*/
    typedef enum
    {
        HTR321x_LED0 = 0,
        HTR321x_LED1,
        HTR321x_LED2,
        HTR321x_LED3,
        HTR321x_LED4,
        HTR321x_LED5,
        HTR321x_LED6,
        HTR321x_LED7,
        HTR321x_LED8,
        HTR321x_LED9,
        HTR321x_LED10,
        HTR321x_LED11,
        HTR321x_LED12,
        HTR321x_LED13,
        HTR321x_LED14,
        HTR321x_LED15,
        HTR321x_LED16,
        HTR321x_LED17,
        HTR321x_LEDMAX = 36
    } htr321x_ledx; //
    /*---------- type define ----------*/
    struct htr321x_event
    {
        enum
        {           
            IOCTL_HTR321x_UPDATA,
            IOCTL_HTR321x_NULL
        } type;
        uint32_t offset;
    };
    typedef struct
    {
        htr321x_ledx  appoint_numx;
        unsigned char toggle_ms;
        unsigned char toggle_times;
    } htr321x_argument;
    typedef struct
    {
        char            *htr321x_iic_name;
        void            *bus;
        uint8_t          address;
        uint8_t          mem_addr_counts; //
        htr321x_argument htr321x_event_ops;
        bool (*init)(void);
        void (*deinit)(void);
        void (*bsp_chip_enable)(void);
        void (*bsp_chip_disable)(void);
        void (*on_event)(struct htr321x_event *evt);
    } htr321x_describe;
    /*---------- variable prototype ----------*/
    typedef htr321x_describe *htr321x_describe_typedef;
    typedef htr321x_argument *htr321x_argument_typedef;

    /*---------- function prototype ----------*/

#ifdef __cplusplus
}
#endif
#endif /* __HTR321X_H */
