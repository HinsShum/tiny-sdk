/**
 * @file common\access_control\serial\inc\serial_transport_level.h
 *
 * Copyright (C) 2022
 *
 * serial_transport_level.h is free software: you can redistribute it and/or modify
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
#ifndef __SERIAL_TRANSPORT_LEVEL_H
#define __SERIAL_TRANSPORT_LEVEL_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "serial_media_access_control.h"

/*---------- macro ----------*/
#define SERIAL_TRANSPORT_RETRANS_MAX_COUNT                  (UINT16_MAX)

/*---------- type define ----------*/
typedef enum {
    SERIAL_TRANSPORT_EX_NONE = 0,
    SERIAL_TRANSPORT_EX_ERROR = -1,
    SERIAL_TRANSPORT_EX_MEMORY_EMPTY = -2
} serial_transport_expection_t;

typedef struct transport *serial_transport_t;

struct serial_transport_ops {
    struct serial_mac_ops mac_ops;
    /* transport level callback interface */
    void (*lock)(void);
    void (*unlock)(void);
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/**
 * @brief Create a new serial transport controller, and return a handle by which the
 * created transport controller can be referenced.
 * @param type Type of access controller at @serial_mac_type_t.
 * @param baudrate Serial hardware baudrate will be configured.
 * @param recv_capacity Trasnport controller will create two receive buffers and one process buffer,
 * each with a size of recv_capacity bytes.
 * @param trans_capacity Transport controller will create one transport buffer with a size of trans_capacity bytes.
 * @param max_blocked_count Max count of blocked packet number at transport level.
 * @param ops Hardware operate callback pointer. It contains serial hardware, timer hardware and event.
 * 
 * @retval If the serial transport controller is sucessfully created then a handle to the newly controller is returned.
 * If the controller cannot be created because there is insufficient heap remaining to allocate the controller
 * structure or hardware initialize failure then NULL is returned.
 */
extern serial_transport_t serial_transport_new(serial_mac_type_t type, uint32_t baudrate, uint32_t recv_capacity,
        uint32_t trans_capacity, uint32_t max_blocked_count, struct serial_transport_ops *ops);

/**
 * @brief Delete a controller that was previously created using the serial_transport_new()
 * API function.
 * @param self The handle of the controller being deleted.
 * 
 * @retval None
 */
extern void serial_transport_delete(serial_transport_t self);

/**
 * @brief Put a series of data to controller's transmitter. But it should be noted that the API does not cached
 * data, it will called the serial hardware interface to send data directly.
 * @param self The handle of the controller being set transmitter.
 * @param pbuf A series of data being set to transmitter.
 * @param length Length of the data being set to transmitter.
 * 
 * @retval None
 */
extern void serial_transport_set_transmitter(serial_transport_t self, const uint8_t *pbuf, uint32_t length);

/**
 * @brief Put a series of data to controller's transmitter. The API different from serial_transport_set_transmitter(),
 * it cached data and will try to re-transport if @retrans_count is not zero and 
 * serial_transport_clear_transmitter() not be called during wait ack period.
 * @param self The handle of the controller being set to transmitter cache.
 * @param pbuf A series of the data being set to transmitter cache.
 * @param length Length of the data being set to transmitter cache.
 * @param retrans_count Re-transport count if not receive ack packet.
 * @param wait_ack_ticks Maximum ticks to wait a ack packet. Only valid for SERIAL_TRANSPORT_TYPE_FULLDUPLEX access controller.
 * 
 * @retval If the controller's transmitter no memory to cached new data, SERIAL_TRANSPORT_EX_MEMORY_EMPTY is returned.
 * If transmitter caced the @pbuf, SERIAL_TRANSPORT_EX_NONE is returned.
 */
extern serial_transport_expection_t serial_transport_set_transmitter_cache(serial_transport_t self, const uint8_t *pbuf,
        uint32_t length, uint16_t retrans_count, uint32_t wait_ack_ticks);

/**
 * @brief Clear controller's transmitter cache.
 * @param self The handle of the controller being clear transmitter.
 * 
 * @retval None
 */
extern void serial_transport_clear_transmitter(serial_transport_t self);

/**
 * @brief When recv a byte by serial hardware, serial_transport_recv_byte() API should be called once.
 * @note API serial_transport_recv_byte() should be regiter to serial hardware interrupt receive server function.
 * @param self The handle of controller.
 * @param byte A byte recevied by serial hardware.
 * 
 * @retval None
 */
extern void serial_transport_recv_byte(serial_transport_t self, uint8_t byte);

/**
 * @brief When timer expired, serial_transport_timer_expired() should be called once.
 * @note API serial_transport_timer_expired() should be register to timer hardware interrupt server function.
 * @param self The handle of controller.
 * 
 * @retval None
 */
extern void serial_transport_timer_expired(serial_transport_t self);

/**
 * @brief Deal with received event and transport event.
 * @param self The handle of controller.
 * 
 * @retval None
 */
extern void serial_transport_poll(serial_transport_t self);

/**
 * @brief Deal with wait ack timeout event.
 * @note API serial_transport_called_per_tick() should be register to system tick interrupt server function.
 * @param self The handle of controller.
 * 
 * @retval None
 */
extern void serial_transport_called_per_tick(serial_transport_t self);

#ifdef __cplusplus
}
#endif
#endif /* __SERIAL_TRANSPORT_LEVEL_H */
