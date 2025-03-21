/**
 * @file inc\options.h
 *
 * Copyright (C) 2021
 *
 * options.h is free software: you can redistribute it and/or modify
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
#ifndef __CONFIG_OPTIONS_H
#define __CONFIG_OPTIONS_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "misc.h"
#include "errorno.h"
#ifdef CONFIG_OPTIONS_FILE
#include CONFIG_OPTIONS_FILE
#endif

/*---------- macro ----------*/
#ifndef xlog_error
#define xlog_error(x, ...)
#endif
#ifndef xlog_cont
#define xlog_cont(x, ...)
#endif

/* assert definition
 */
#undef assert
#ifdef NDEBUG
#define assert(expr)                    ((void)0U)
#else
#ifndef __FILE_NAME__
#define __FILE_NAME__                   __FILE__
#endif
#define assert(expr)                  	do { \
                                            if(!(expr)) { \
                                                xlog_error("Assert in %s:%d\n", __FILE_NAME__, __LINE__); \
                                                for(;;); \
                                            } \
                                        } while(0)
#endif

/* buffer content print definition
 */
#ifndef PRINT_BUFFER_CONTENT
#define PRINT_BUFFER_CONTENT(color, tag, buf, length)   \
        do {                                            \
            if(!length) {                               \
                break;                                  \
            }                                           \
            xlog_cont("%s%s: ", color, tag);            \
            for(uint32_t i = 0; i < length; ++i) {      \
                xlog_cont("%02X ", buf[i]);             \
            }                                           \
            xlog_cont("\b\n");                          \
        } while(0);
#endif

/*---------- type define ----------*/
/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/

#ifdef __cplusplus
}
#endif
#endif /* __CONFIG_OPTIONS_H */
