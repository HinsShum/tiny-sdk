/**
 * @file common/utils/xlog/inc/xlog.h
 *
 * Copyright (C) 2022
 *
 * xlog.h is free software: you can redistribute it and/or modify
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
#ifndef __XLOG_H
#define __XLOG_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/*---------- macro ----------*/
/* log level definition 
 */
#define LOG_ERROR                           "<0>"   /*<< error conditions */
#define LOG_WARN                            "<1>"   /*<< warining conditions */
#define LOG_MESSAGE                         "<2>"   /*<< message conditions */
#define LOG_INFO                            "<3>"   /*<< informational */
#define LOG_DEFAULT                         "<d>"   /*<< use the default log level */
/* Annotation for a "continuted" line of log printout(only done after a
 * line that had no enclasing \n). Only to be used by core/arch code during
 * early bootup(a continued line is not SMP=safe otherwise).
 */
#define LOG_CONT                            "<c>"

/* xlog API functions definition
 */
#define xlog_error(x, y...)                 xlog(LOG_ERROR x, ##y)
#define xlog_warn(x, y...)                  xlog(LOG_WARN x, ##y)
#define xlog_message(x, y...)               xlog(LOG_MESSAGE x, ##y)
#define xlog_info(x, y...)                  xlog(LOG_INFO x, ##y)
#define xlog_cont(x, y...)                  xlog(LOG_CONT x, ##y)
#define xlog_tag_error(tag, x, y...)        xlog(LOG_ERROR "(" tag ")" x, ##y)
#define xlog_tag_warn(tag, x, y...)         xlog(LOG_WARN "(" tag ")" x, ##y)
#define xlog_tag_message(tag, x, y...)      xlog(LOG_MESSAGE "(" tag ")" x, ##y)
#define xlog_tag_info(tag, x, y...)         xlog(LOG_INFO "(" tag ")" x, ##y)

/*---------- type define ----------*/
typedef struct {
    void (*lock)(void);
    void (*unlock)(void);
    bool (*acquire_console)(void);
    void (*release_console)(void);
    void (*get_timestamp)(time_t *utc);
    void (*print)(const char *str, uint32_t length);
} xlog_ops_t;

typedef void (*xlog_print_func_t)(const char *str, uint32_t length);

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/**
 * @brief Print a message to the console.
 * One effect of this deferred printing is that code which calls xlog() and
 * then changes xlog level may break. This is because xlog level is inspected
 * when the actual printing accurs.
 * @param fmt Format string.
 * 
 * @retval The length actually printed or put into the log buffer.
 */
#ifdef CONFIG_USE_XLOG
extern uint32_t __attribute__((format(printf, 1, 0))) xlog(const char *fmt, ...);
#else
#define xlog(x, y...)
#endif

/**
 * @brief Set function used to output log entries. 
 * @param print New function used for output.
 * 
 * @retval Function old is returned.
 */
extern xlog_print_func_t xlog_set_print_func(xlog_print_func_t print);

/**
 * @brief Set log level for determine which log can output.
 * @param level One of the following parameters: LOG_ERROR, LOG_WARN, LOG_MESSAGE
 * and LOG_INFO. Only log level at this or lower verbosity levels will be shown.
 * 
 * @retval Set log level successfully then true is returned, otherwise false is
 * returned.
 */
extern bool xlog_set_log_level(const char *level);

/**
 * @brief Control if the log type can output. No log type is output by default.
 * @param hide If hide is true, no log type is output, otherwise the log type
 * is output.
 * 
 * @retval None
 */
extern void xlog_hide_log_type(bool hide);

/**
 * @brief Initialize xlog.
 * @param ops API functions structure for xlog use.
 * If use xlog in multi-thread os, must to implement lock(), unlock(), acquire_console(),
 * release_console() API functions to protect the log buffer and console.
 * lock() and unlock() API functions to protect the log buffer, it can be impemented from
 * mutex lock.
 * acquire_console() and release_console() API functions to protect the console, it can
 * be implemented from binary semaphore.
 * print() API function must be implemented to output message.
 * 
 * @retval None
 */
extern void xlog_init(xlog_ops_t *ops);

/**
 * @brief Deinitialize xlog.
 * 
 * @retval None
 */
extern void xlog_deinit(void);

#ifdef __cplusplus
}
#endif
#endif /* __XLOG_H */
