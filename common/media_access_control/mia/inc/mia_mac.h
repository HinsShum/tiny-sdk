/**
 * @file media_access_control\mia\inc\mia_mac.h
 *
 * Copyright (C) 2023
 *
 * mia_mac.h is free software: you can redistribute it and/or modify
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
#ifndef __MIA_MAC_H
#define __MIA_MAC_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "mia_phy.h"

/*---------- macro ----------*/
/*---------- type define ----------*/
typedef enum {
    MIA_MAC_EX_TRANS_BUSY = -2,
    MIA_MAC_EX_ERROR = -1,
    MIA_MAC_EX_NONE = 0,
} mia_mac_expection_t;

typedef enum {
    MIA_MAC_EVT_RECEIVED,
    MIA_MAC_EVT_BUS_FAULT,
    MIA_MAC_EVT_BUS_RECOVER,
    MIA_MAC_EVT_TRANSMITTER_READY,
} mia_mac_evt_t;

typedef struct mia_mac_ops *mia_mac_ops_t;
struct mia_mac_ops {
    uint32_t disf;                  /*<< bus silence time */
    /* tx rx callback interface */
    bool (*set_tx_bit)(bool bit);
    bool (*get_rx_bit)(void);
    void (*monitor_start_bit)(bool enable);
    bool (*io_init)(void);
    /* timer callback interface */
    bool (*timer_init)(uint32_t us);
    void (*timer_ctrl)(bool enable);
    /* event callback interface */
    bool (*event_init)(void);
    bool (*event_post)(mia_mac_evt_t evt, bool protected);
    bool (*event_get)(mia_mac_evt_t *pevt);
    /* mia bus fault callback interface */
    void (*bus_fault)(bool fault);
    /* packet parse callback interface */
    void (*receive_packet_parse)(const uint8_t *recv_buf, uint32_t recv_length, const uint8_t *trans_buf, uint32_t trans_length);
};

typedef struct mac *mia_mac_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
extern mia_mac_t mia_mac_new(uint32_t baudrate, uint32_t recv_capacity, uint32_t trans_capacity, mia_mac_ops_t ops);
extern void mia_mac_delete(mia_mac_t self);
extern void mia_mac_set_transmitter(mia_mac_t self, const uint8_t *pbuf, uint32_t length);
extern mia_mac_expection_t mia_mac_set_transmitter_cache(mia_mac_t self, const uint8_t *pbuf, uint32_t length, uint16_t retrans_count);
extern void mia_mac_clear_transmitter(mia_mac_t self);
extern void mia_mac_start_bit_detected(mia_mac_t self);
extern void mia_mac_timer_expired(mia_mac_t self);
extern void mia_mac_polling(mia_mac_t self);
extern void mia_mac_called_per_tick(mia_mac_t self);

#ifdef __cplusplus
}
#endif
#endif /* __MIA_MAC_H */
