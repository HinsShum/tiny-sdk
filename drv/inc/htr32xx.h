/**
 * @file drv\inc\htr32xx.h
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
#ifndef __HTR32XX_H
#define __HTR32XX_H

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
#define HTR3236_NAME  "HTR3236"   
#define HTR3218_NAME  "HTR3218"    
#define HTR3212_NAME  "HTR3212"    

typedef enum
{
    HTR32xx_LED0 = 0,
    HTR32xx_LED1,
    HTR32xx_LED2,
    HTR32xx_LED3,
    HTR32xx_LED4,
    HTR32xx_LED5,
    HTR32xx_LED6,
    HTR32xx_LED7,
    HTR32xx_LED8,
    HTR32xx_LED9,
    HTR32xx_LED10,
    HTR32xx_LED11,
    HTR32xx_LED12,
    HTR32xx_LED13,
    HTR32xx_LED14,
    HTR32xx_LED15,
    HTR32xx_LED16,
    HTR32xx_LED17,
    HTR32xx_LED18,
    HTR32xx_LED19,
    HTR32xx_LED20,
    HTR32xx_LED21,
    HTR32xx_LED22,
    HTR32xx_LED23,
    HTR32xx_LED24,
    HTR32xx_LED25,
    HTR32xx_LED26,
    HTR32xx_LED27,
    HTR32xx_LED28,
    HTR32xx_LED29,
    HTR32xx_LED30,
    HTR32xx_LED31,
    HTR32xx_LED32,
    HTR32xx_LED33,
    HTR32xx_LED34,
    HTR32xx_LED35,
    HTR32xx_LED36,
    HTR32xx_LEDMAX = HTR32xx_LED36
} htr32xx_ledx; //
/*---------- type define ----------*/
struct htr32xx_event
{
    enum
    {
        IOCTL_HTR32xx_UPDATA,
        IOCTL_HTR32xx_NULL
    } type;
    uint32_t offset;
};
typedef struct
{
    htr32xx_ledx  appoint_numx;
    unsigned char toggle_ms;
    unsigned char toggle_times;
} htr32xx_argument;
typedef struct
{
    char* htr32xx_iic_name;
    char* htr32xx_dev_name;
    void* bus;
    uint8_t          address;
    uint8_t          htr32xx_ledx_max;
    uint8_t          mem_addr_counts; //
    htr32xx_argument htr32xx_event_ops;
    bool (*init)(void);
    void (*deinit)(void);
    void (*bsp_chip_enable)(void);
    void (*bsp_chip_disable)(void);
    void (*on_event)(struct htr32xx_event* evt);
} htr32xx_describe;
/*---------- variable prototype ----------*/
typedef htr32xx_describe* htr32xx_describe_typedef;
typedef htr32xx_argument* htr32xx_argument_typedef;

/*---------- function prototype ----------*/

#ifdef __cplusplus
}
#endif
#endif /* __htr32xX_H */
