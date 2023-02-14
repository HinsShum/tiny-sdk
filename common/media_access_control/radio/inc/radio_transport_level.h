/**
 * @file common\access_control\radio\inc\radio_transport_level.h
 *
 * Copyright (C) 2022
 *
 * radio_transport_level.h is free software: you can redistribute it and/or modify
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
#ifndef __RADIO_TRANSPORT_LEVEL_H
#define __RADIO_TRANSPORT_LEVEL_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "radio_media_access_control.h"

/*---------- macro ----------*/
#define RADIO_TRANSPORT_RETRANS_MAX_COUNT                       (UINT16_MAX)

/*---------- type define ----------*/
typedef enum {
    RADIO_TRANSPORT_EX_NONE = 0,
    RADIO_TRANSPORT_EX_ERROR = -1,
    RADIO_TRANSPORT_EX_MEMORY_EMPTY = -2
} radio_transport_expection_t;

typedef struct transport *radio_transport_t;

struct radio_transport_ops {
    struct radio_mac_ops mac_ops;
    void (*lock)(void);
    void (*unlock)(void);
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/**
 * @brief Create a new radio transport controller and return a handle by which the
 * created transport controller can be referenced.
 * @param recv_capacity Transport controller will create two receive buffer and one
 * process buffer, each with a size of recv_capacity bytes.
 * @param trans_capacity Transport controller will create one transport buffer with
 * a size of trans_capacity bytes.
 * @param max_blocked_count Max count of blocked packet number at transport level.
 * @param ops Hardware operate callback pointer.It contains radio hardware and event.
 * 
 * @retval If the radio transport controller is successfully created then a handle to
 * newly controller is returned. If the controller cannot be created because there is
 * insufficient heap remaining to allocate the controller structure or event initialize
 * failure then NULL is returned.
 */
extern radio_transport_t radio_transport_new(uint32_t recv_capacity, uint32_t trans_capacity,
        uint32_t max_blocked_count, struct radio_transport_ops *ops);

/**
 * @brief Delete a controller that was previously created using the radio_transport_new()
 * API function.
 * @param self The handle of the controller being deleted.
 * 
 * @retval None
 */
extern void radio_transport_delete(radio_transport_t self);

/**
 * @brief Put a series of data to controller's transmitter. But is should be noted that the
 * API does not cached data, it will called the radio hardware interface to send data directly.
 * @param self The handle of the controller being set transmitter.
 * @param pbuf A series of data being set to transmitter.
 * @param length Length of the data being set to transmitter.
 * 
 * @retval None
 */
extern void radio_transport_set_transmitter(radio_transport_t self, const uint8_t *pbuf, uint32_t length);

/**
 * @brief Put a series of data to controller's transmitter. The API different from
 * radio_transport_set_transmitter(), it cached data and will try re-transport if @retrans_count
 * is not zero and radio_transport_clear_transmitter() not be called during wait ack period.
 * @param self The handle of the controller being set to transmitter cache.
 * @param pbuf A series of the data being set to transmitter cache.
 * @param length Length of the data being set to transmitter cache.
 * @param retrans_count Re-transport count if not receive ack packet.
 * 
 * @retval If the controller's transmitter no memory to cached new data, RADIO_TRANSPORT_EX_MEMORY_EMPTY
 * is returned. If transmitter cached the @pbuf, RADIO_TRANSPORT_EX_NONE is returned.
 */
extern radio_transport_expection_t radio_transport_set_transmitter_cache(radio_transport_t self, const uint8_t *pbuf,
        uint32_t length, uint16_t retrans_count);

/**
 * @brief Clear controller's transmitter cache.
 * @param self The handle of the controller being clear transmitter.
 * 
 * @retval None
 */
extern void radio_transport_clear_transmitter(radio_transport_t self);

/**
 * @brief Post an event to the transport level controller.
 * @param self The handle of the controller.
 * @param evt An event to post.
 * @param protected If called the radio_transport_event_post() API in the interrupt, protected must be true,
 * otherwise protected must be false.
 * 
 * @retval None
 */
extern void radio_transport_event_post(radio_transport_t self, radio_mac_evt_t evt, bool protected);

/**
 * @brief Deal with received event and transport event.
 * @param self The handle of the controller.
 * 
 * @retval None
 */
extern void radio_transport_poll(radio_transport_t self);

/**
 * @brief Deal with wait ack timeout event.
 * @note API radio_transport_called_per_tick() should be register to system tick interrupt server function.
 * @param self The handle of the controller.
 * 
 * @retval None
 */
extern void radio_transport_called_per_tick(radio_transport_t self);

#ifdef __cplusplus
}
#endif
#endif /* __RADIO_TRANSPORT_LEVEL_H */
