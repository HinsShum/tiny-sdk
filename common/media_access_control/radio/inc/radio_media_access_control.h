/**
 * @file common\access_control\radio\inc\radio_media_access_control.h
 *
 * Copyright (C) 2022
 *
 * radio_media_access_control.h is free software: you can redistribute it and/or modify
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
#ifndef __RADIO_ACCESS_CONTROL_H
#define __RADIO_ACCESS_CONTROL_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*---------- macro ----------*/
/*---------- type define ----------*/
typedef enum {
    RADIO_MAC_EX_NONE = 0,
    RADIO_MAC_EX_ERROR = -1,
    RADIO_MAC_EX_TRANS_BUSY = -2
} radio_mac_expection_t;

typedef enum {
    RADIO_MAC_EVT_RECEIVED,
    RADIO_MAC_EVT_RECEIVING,
    RADIO_MAC_EVT_TRANSMITTER_READY,
    RADIO_MAC_EVT_BUS_TRY_LOCK,
    RADIO_MAC_EVT_BUS_TRY_UNLOCK,
    RADIO_MAC_EVT_CUSTOM_START
} radio_mac_evt_t;

typedef struct mac *radio_mac_t;

struct radio_mac_ops {
    /* radio callback interface */
    uint32_t (*radio_receive)(uint8_t *pbuf, uint32_t capacity, bool continuing);
    void (*radio_post)(const uint8_t *pbuf, uint32_t length);
    /* event callback interface */
    bool (*event_init)(void);
    void (*event_post)(radio_mac_evt_t evt, bool protected);
    bool (*event_get)(radio_mac_evt_t *pevt);
    /* packet parse callback interface */
    void (*receive_packet_parse)(const uint8_t *recv_buf, uint32_t recv_length, const uint8_t *trans_buf, uint32_t trans_length);
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/**
 * @brief Create a new radio media access controller and return a handle by which the 
 * created media access controller can be referenced.
 * @param recv_capacity Media access controller will create two receive buffers and one
 * process buffer, each with a size of recv_capacity bytes.
 * @param trans_capacity Media access controller will create one transport buffer with
 * a size of trans_capacity bytes.
 * @param ops Hardware operate callback pointer. It contains radio hardware and event.
 * 
 * @retval If the media access controller is successfully created then a handle to the
 * newly controller is returned. If the controller cannot be created because there is
 * insufficient heap remaining to allocate the controller structure or event initialize
 * failure then NULL is returned.
 */
extern radio_mac_t radio_mac_new(uint32_t recv_capacity, uint32_t trans_capacity, struct radio_mac_ops *ops);

/**
 * @brief Delete a controller that was preciously created using the radio_mac_new()
 * API function.
 * @param self The handle of the controller being deleted.
 * 
 * @retval None
 */
extern void radio_mac_delete(radio_mac_t self);

/**
 * @brief Put a series of data to controller's transmitter. But it should be noted that
 * the API does not cached data, it will called the radio hardware interface to send
 * data directly.
 * @param self The handle of the conrtoller being set transmitter.
 * @param pbuf A series of data being set to transmitter.
 * @param length Length of the data being set to transmitter.
 * 
 * @retval None
 */
extern void radio_mac_set_transmitter(radio_mac_t self, const uint8_t *pbuf, uint32_t length);

/**
 * @brief Put a series of data to controller's transmitter. The API different from 
 * radio_mac_set_transmitter(), it cached data and will try to re-transport if @retrans_count is
 * not zero and radio_mac_clear_transmitter() not be called during wait ack period.
 * @param self The handle of the controller being set to transmitter cache.
 * @param pbuf A series of data being set to transmitter cache.
 * @param length Length of the data being set to transmitter cache.
 * @param retrans_count Re-transport count if not receive ack packet.
 * 
 * @retval If the controller's transmitter has cached data, RADIO_MAC_EX_TRANS_BUSY is returned.
 * If @length large then transmitter buffer capacity, RADIO_MAC_EX_ERROR is returned.
 * If transmitter cached the @pbbuf, RADIO_MAC_EX_NONE is returned.
 */
extern radio_mac_expection_t radio_mac_set_transmitter_cache(radio_mac_t self, const uint8_t *pbuf,
        uint32_t length, uint16_t retrans_count);

/**
 * @brief Clear controller's transmitter cache.
 * @param self The handle of the conrtoller being clear transmitter.
 * 
 * @retval None
 */
extern void radio_mac_clear_transmitter(radio_mac_t self);

/**
 * @brief Post an event to the radio mmedia access controller.
 * @param self The handle of the controller being post an event.
 * @param evt An event to post.
 * @param protected If called the radio_mac_event_post() API in the interrupt, protected must be true,
 * otherwise protected must be false.
 * 
 * @retval None
 */
extern void radio_mac_event_post(radio_mac_t self, radio_mac_evt_t evt, bool protected);

/**
 * @brief Deal with received event and transport event.
 * @param self The handle of the controller.
 * 
 * @retval None
 */
extern void radio_mac_poll(radio_mac_t self);

/**
 * @brief Deal with wait ack timeout event.
 * @note API radio_mac_called_per_tick() should be register to system tick interrupt server function.
 * @param self The handle of the controller.
 * 
 * @retval None
 */
extern void radio_mac_called_per_tick(radio_mac_t self);

#ifdef __cplusplus
}
#endif
#endif /* __RADIO_ACCESS_CONTROL_H */
