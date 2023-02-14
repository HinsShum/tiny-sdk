/**
 * @file common/utils/xlog/xlog.c
 *
 * Copyright (C) 2022
 *
 * xlog.c is free software: you can redistribute it and/or modify
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

/*---------- includes ----------*/
#include "xlog.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef CONFIG_USE_XLOG
/*---------- macro ----------*/
/* log buffer defitions
 */
#ifndef CONFIG_XLOG_BUF_SHIFT
#define CONFIG_XLOG_BUF_SHIFT               (10)
#endif
#define __LOG_BUF_LEN                       (1UL << CONFIG_XLOG_BUF_SHIFT)
#define LOG_BUF_MASK                        (__LOG_BUF_LEN - 1)
#define LOG_BUF(off)                        (log_buf[(off) & LOG_BUF_MASK])

/* default log level
 */
#define DEFAULT_MESSAGE_LOG_LEVEL           (1)     /*<< LOG_WARN */
#define DEFAULT_CONSOLE_LOG_LEVEL           (4)     /*<< anything more serious than LOG_INFO */

/*---------- type define ----------*/
struct xlog_describe {
    struct {
        uint32_t default_level;
        uint32_t console_level;
    } log_level;
    bool hide_log_type;
    xlog_ops_t ops;
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/*---------- variable ----------*/
static struct xlog_describe _xlog;
static uint32_t log_start = 0;                      /*<< Index into log_buf: next char to be sent to consoles */
static uint32_t log_end = 0;                        /*<< Index into log_buf: most-recenrly-written + 1 */
static char log_buf[__LOG_BUF_LEN];
static bool next_text_line = true;
static char vprintf_buf[__LOG_BUF_LEN];
static char log_level_char[] = {
    [0] = 'E',
    [1] = 'W',
    [2] = 'M',
    [3] = 'I'
};
static char *log_level_color[] = {
    [0] = "\033[31;22m",
    [1] = "\033[33;22m",
    [2] = "\033[32;22m",
    [3] = "\033[37;22m"
};

/*---------- function ----------*/
static inline void __lock(void)
{
    if(_xlog.ops.lock) {
        _xlog.ops.lock();
    }
}

static inline void __unlock(void)
{
    if(_xlog.ops.unlock) {
        _xlog.ops.unlock();
    }
}

static inline bool __acquire_console(void)
{
    bool retval = true;

    if(_xlog.ops.acquire_console) {
        retval = _xlog.ops.acquire_console();
    }

    return retval;
}

static inline void __release_console(void)
{
    if(_xlog.ops.release_console) {
        _xlog.ops.release_console();
    }
}

static inline void __console_print(uint32_t start, uint32_t end)
{
    if(_xlog.ops.print) {
        _xlog.ops.print(&LOG_BUF(start), end - start);
    }
}

static void emit_log_char(char c)
{
    uint32_t cache_len = 0;

    LOG_BUF(log_end) = c;
    log_end++;
    /* buf overflow, drop some str */
    if(log_end < log_start) {
        cache_len = UINT32_MAX - log_start + log_end + 1;
    } else {
        cache_len = log_end - log_start;
    }
    if(cache_len > __LOG_BUF_LEN) {
        log_start++;
    }
}

static void __call_console(uint32_t start, uint32_t end, uint32_t log_level)
{
    if(log_level < _xlog.log_level.console_level && start != end) {
        if((start & LOG_BUF_MASK) > (end & LOG_BUF_MASK)) {
            __console_print(start & LOG_BUF_MASK, __LOG_BUF_LEN);
            __console_print(0, end & LOG_BUF_MASK);
        } else {
            __console_print(start, end);
        }
    }
}

static void _call_console(uint32_t start, uint32_t end)
{
    uint32_t cur_off = start, print_off = start;
    static int32_t msg_level = -1;
    bool overflow = false;

    if((int32_t)(start - end) > 0) {
        overflow = true;
    }
    while(cur_off != end) {
        if((msg_level < 0) &&
                ((!overflow && ((end - cur_off) > 2)) ||
                (overflow && ((UINT32_MAX - cur_off + end + 1) > 2))) &&
                LOG_BUF(cur_off + 0) == '<' &&
                LOG_BUF(cur_off + 1) >= '0' &&
                LOG_BUF(cur_off + 1) <= '3' &&
                LOG_BUF(cur_off + 2) == '>') {
            msg_level = LOG_BUF(cur_off + 1) - '0';
            LOG_BUF(cur_off + 1) = log_level_char[msg_level];
            cur_off += 3;
            print_off += 3;
        }
        while(cur_off != end) {
            char c = LOG_BUF(cur_off);
            cur_off++;
            if(c == '\n') {
                if(msg_level < 0) {
                    msg_level = _xlog.log_level.default_level;
                }
                __call_console(print_off, cur_off, msg_level);
                msg_level = -1;
                print_off = cur_off;
                break;
            }
        }
    }
    __call_console(print_off, end, msg_level);
}

static bool _acquire_console(void)
{
    __unlock();
    
    return __acquire_console();
}

static void _print_and_release_console(void)
{
    uint32_t _con_start = log_start, _con_end = log_end;

    log_start = log_end;
    _call_console(_con_start, _con_end);
    __release_console();
}

static inline uint32_t _vscnprint(char *buf, uint32_t size, const char *fmt, va_list args)
{
    uint32_t len = 0;

    len = vsnprintf(buf, size, fmt, args);

    return (len >= size) ? (size - 1) : len;
}

static uint32_t __attribute__((format(printf, 1, 0))) _vprint(const char *fmt, va_list args)
{
    uint32_t printed_len = 0;
    uint32_t cur_log_level = _xlog.log_level.default_level;
    char *p = NULL;
    char c = 0;
    uint32_t len = 0;

    __lock();
    printed_len = _vscnprint(vprintf_buf, sizeof(vprintf_buf), fmt, args);
    p = vprintf_buf;
    /* Do we have a log level in the string? */
    if(p[0] == '<') {
        c = p[1];
        if(c && p[2] == '>') {
            switch(c) {
                case '0'...'3':
                    /* Get log level */
                    cur_log_level = c - '0';
                case 'd':
                    /* Failthrough - make sure we're on a new line */
                    if(!next_text_line) {
                        emit_log_char('\n');
                        printed_len += 1;
                        next_text_line = true;
                    }
                case 'c':
                    /* Failthrough - skip the log level */
                    p += 3;
                    printed_len -= 3;
                    break;
            }
        }
    }
    for(; *p; ++p) {
        if(next_text_line) {
            emit_log_char('<');
            emit_log_char(cur_log_level + '0');
            emit_log_char('>');
            printed_len += 3;
            /* coloring */
            len = strlen(log_level_color[cur_log_level]);
            for(uint32_t i = 0; i < len; ++i) {
                emit_log_char(log_level_color[cur_log_level][i]);
            }
            printed_len += len;
            /* timestamp */
            if(_xlog.ops.get_timestamp) {
                time_t utc = 0;
                struct tm *ptm = NULL;
                char time_str[30] = {0};
                _xlog.ops.get_timestamp(&utc);
                ptm = localtime(&utc);
                strftime(time_str, sizeof(time_str), "[%Y-%m-%d %H:%M:%S]", ptm);
                len = strlen(time_str);
                for(uint32_t i = 0; i < len; ++i) {
                    emit_log_char(time_str[i]);
                }
                printed_len += len;
            }
            if(!_xlog.hide_log_type) {
                /* log type */
                emit_log_char('<');
                emit_log_char(log_level_char[cur_log_level]);
                emit_log_char('>');
                printed_len += 3;
            }
            next_text_line = false;
        }
        emit_log_char(*p);
        if(*p == '\n') {
            next_text_line = true;
        }
    }
    if(_acquire_console()) {
        _print_and_release_console();
    }

    return printed_len;
}

uint32_t __attribute__((format(printf, 1, 0))) xlog(const char *fmt, ...)
{
    va_list args;
    uint32_t len = 0;

    va_start(args, fmt);
    len = _vprint(fmt, args);
    va_end(args);

    return len;
}

xlog_print_func_t xlog_set_print_func(xlog_print_func_t print)
{
    xlog_print_func_t old_print = _xlog.ops.print;

    _xlog.ops.print = print;

    return old_print;
}

bool xlog_set_log_level(const char *level)
{
    bool retval = false;

    if(level[0] == '<' && level[1] >= '0' && level[1] <= '3' && level[2] == '>') {
        _xlog.log_level.console_level = (level[1] - '0') + 1;
        retval = true;
    }

    return retval;
}

void xlog_hide_log_type(bool hide)
{
    _xlog.hide_log_type = hide;
}

void xlog_init(xlog_ops_t *ops)
{
    log_start = 0;
    log_end = 0;
    next_text_line = true;
    _xlog.log_level.default_level = DEFAULT_MESSAGE_LOG_LEVEL;
    _xlog.log_level.console_level = DEFAULT_CONSOLE_LOG_LEVEL;
    _xlog.hide_log_type = true;
    if(ops) {
        _xlog.ops = *ops;
    }
}

void xlog_deinit(void)
{
    _xlog.ops.lock = NULL;
    _xlog.ops.unlock = NULL;
    _xlog.ops.acquire_console = NULL;
    _xlog.ops.release_console = NULL;
    _xlog.ops.print = NULL;
}
#else
xlog_print_func_t xlog_set_print_func(xlog_print_func_t print)
{
    (void)print;

    return print;
}

bool xlog_set_log_level(const char *level)
{
    (void)level;

    return true;
}

void xlog_hide_log_type(bool hide)
{
    (void)hide;
}

void xlog_init(xlog_ops_t *ops)
{
    (void)ops;
}

void xlog_deinit(void)
{
}

#endif
