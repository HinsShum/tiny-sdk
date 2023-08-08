/**
 * @file media_access_control\mia\inc\mia_transport_level.h
 *
 * Copyright (C) 2023
 *
 * mia_transport_level.h is free software: you can redistribute it and/or modify
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
#ifndef __MIA_TRANSPORT_LEVEL_H
#define __MIA_TRANSPORT_LEVEL_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "mia_mac.h"

/*---------- macro ----------*/
#define MIA_TRANSPORT_RETRANS_MAX_COUNT                     (UINT16_MAX)

/*---------- type define ----------*/
typedef enum {
    MIA_TRANSPORT_EX_NONE = 0,
    MIA_TRANSPORT_EX_ERROR = -1,
    MIA_TRANSPORT_EX_MEMORY_EMPTY = -2,
} mia_transport_expection_t;

typedef struct mia_transport_ops *mia_transport_ops_t;
struct mia_transport_ops {
    struct mia_mac_ops mac_ops;
    /* transport buffer lock/unlock */
    void (*lock)(void);
    void (*unlock)(void);
};

typedef struct mia_transport *mia_transport_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
extern mia_transport_t mia_transport_new(uint32_t baudrate, uint32_t recv_capacity, uint32_t trans_capacity,
        uint32_t max_blocked_count, mia_transport_ops_t ops);
extern void mia_transport_delete(mia_transport_t self);
extern void mia_transport_set_transmitter(mia_transport_t self, const uint8_t *pbuf, uint32_t length);
extern mia_transport_expection_t mia_transport_set_transmitter_cache_low(mia_transport_t self,
        const uint8_t *pbuf, uint32_t length, uint16_t retrans_count);
extern mia_transport_expection_t mia_transport_set_transmitter_cache_high(mia_transport_t self,
        const uint8_t *pbuf, uint32_t length, uint16_t retrans_count);
extern void mia_transport_clear_transmitter(mia_transport_t self);
extern void mia_transport_start_bit_detected(mia_transport_t self);
extern void mia_transport_timer_expired(mia_transport_t self);
extern void mia_transport_called_per_tick(mia_transport_t self);
extern void mia_transport_polling(mia_transport_t self);

#ifdef __cplusplus
}
#endif
#endif /* __MIA_TRANSPORT_LEVEL_H */
