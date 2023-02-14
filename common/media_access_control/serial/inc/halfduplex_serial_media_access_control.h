/**
 * @file common\access_control\serial\inc\halfduplex_serial_media_access_control.h
 *
 * Copyright (C) 2022
 *
 * halfduplex_serial_media_access_control.h is free software: you can redistribute it and/or modify
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
#ifndef __HALFDUPLEX_SERIAL_MEDIA_ACCESS_CONTROL_H
#define __HALFDUPLEX_SERIAL_MEDIA_ACCESS_CONTROL_H

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
/*---------- type define ----------*/
/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/**
 * @brief Create a new half duplex serial media access controller, and return a handle by which the
 * created media access controller can be referenced.
 * @param baudrate Serial hardware baudrate will be configured.
 * @param recv_capacity Media access controller will create two receive buffers and one process buffer,
 * each with a size of recv_capacity bytes.
 * @param trans_capacity Media access controller will create one transport buffer with a size of trans_capacity bytes.
 * @param ops Hardware operate callback pointer. It contains serial hardware, timer hardware and event.
 * 
 * @retval If the media access controller is sucessfully created the a handle to the newly controller is returned.
 * If the controller cannot be created because there is insufficient heap remaining to allocate the controller
 * structure or hardware initialize failure then NULL is returned.
 */
extern serial_mac_t halfduplex_serial_media_access_controller_new(uint32_t baudrate, uint32_t recv_capacity,
        uint32_t trans_capacity, struct serial_mac_ops *ops);

/**
 * @brief Delete a controller that was previously created using the halfduplex_serial_media_access_controller_new()
 * API function.
 * @param self The handle of the controller being deleted.
 * 
 * @retval None
 */
extern void halfduplex_serial_media_access_controller_delete(serial_mac_t self);

/**
 * @brief Put a series of data to controller's transmitter. But it should be noted that the API does not cached
 * data, it will called the serial hardware interface to send data directly.
 * @param self The handle of the controller being set transmitter.
 * @param pbuf A series of data being set to transmitter.
 * @param length Length of the data being set to transmitter.
 * 
 * @retval None
 */
extern void halfduplex_serial_mac_set_transmitter(serial_mac_t self, const uint8_t *pbuf, uint32_t length);

/**
 * @brief Put a series of data to controller's transmitter. The API different from halfduplex_serial_mac_set_transmitter(),
 * it cached data and will try to re-transport if @retrans_count is not zero and 
 * halfduplex_serial_mac_clear_transmitter() not be called during wait ack period.
 * @param self The handle of the controller being set to transmitter cache.
 * @param pbuf A series of the data being set to transmitter cache.
 * @param length Length of the data being set to transmitter cache.
 * @param retrans_count Re-transport count if not receive ack packet.
 * @param no_use Not used
 * 
 * @retval If the controller's transmitter has caced data, SERIAL_MAC_EX_TRANS_BUSY is returned.
 * If @length large than transmitter buffer capacity, SERIAL_MAC_EX_ERROR is returned.
 * If transmitter caced the @pbuf, SERIAL_MAC_EX_NONE is returned.
 */
extern serial_mac_expection_t halfduplex_serial_mac_set_transmitter_cache(serial_mac_t self, const uint8_t *pbuf,
        uint32_t length, uint16_t retrans_count, uint32_t no_use);

/**
 * @brief Clear controller's transmitter cache.
 * @param self The handle of the controller being clear transmitter.
 * 
 * @retval None
 */
extern void halfduplex_serial_mac_clear_transmitter(serial_mac_t self);

/**
 * @brief When recv a byte by serial hardware, halfduplex_serial_mac_recv_byte() API should be called once.
 * @note API halfduplex_serial_mac_recv_byte() should be regiter to serial hardware interrupt receive server function.
 * @param self The handle of controller.
 * @param byte A byte recevied by serial hardware.
 * 
 * @retval None
 */
extern void halfduplex_serial_mac_recv_byte(serial_mac_t self, uint8_t byte);

/**
 * @brief When timer expired, halfduplex_serial_mac_timer_expired() should be called once.
 * @note API halfduplex_serial_mac_timer_expired() should be register to timer hardware interrupt server function.
 * @param self The handle of controller.
 * 
 * @retval None
 */
extern void halfduplex_serial_mac_timer_expired(serial_mac_t self);

/**
 * @brief Deal with received event and transport event.
 * @param self The handle of controller.
 * 
 * @retval None
 */
extern void halfduplex_serial_mac_poll(serial_mac_t self);

/**
 * @brief Deal with wait ack timeout event.
 * @note API halfduplex_serial_mac_called_per_tick() should be register to system tick interrupt server function.
 * @param self The handle of controller.
 * 
 * @retval None
 */
extern void halfduplex_serial_mac_called_per_tick(serial_mac_t self);

#ifdef __cplusplus
}
#endif
#endif /* __HALFDUPLEX_SERIAL_MEDIA_ACCESS_CONTROL_H */
