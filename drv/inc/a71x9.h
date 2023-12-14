/**
 * @file components\tiny-sdk\drv\inc\a71x9.h
 *
 * Copyright (C) 2023
 *
 * a71x9.h is free software: you can redistribute it and/or modify
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
#ifndef __A71X9_H
#define __A71X9_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------- includes ----------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "device.h"

/*---------- macro ----------*/
/**
 * @brief Reinitialize the a71x9, it will reset the a71x9,
 *        then write configure data and write id.
 * @param Args is not used, it can be NULL.
 * @retval If reinitialize a71x9 successful, the interface will
 *         return CY_EOK, otherwise will return CY_ERROR.
 */
#define IOCTL_A71X9_REINITIALIZE                            (IOCTL_USER_START + 0x00)

/**
 * @brief Configure a71x9 to start receiving packet.
 * @param Args is not used, it can be NULL.
 * @retval If configure success, it will return CY_EOK, otherwise
 *         return CY_ERROR.
 */
#define IOCTL_A71X9_START_RECEIVING                         (IOCTL_USER_START + 0x01)

/**
 * @brief Clear a71x9 receiver fifo, drop all data received.
 * @param Args is not used, it can be NULL.
 * @retval If clear success, it will return CY_EOK, otherwise
 *         return CY_ERROR.
 */
#define IOCTL_A71X9_CLEAR_RECEIVER_FIFO                     (IOCTL_USER_START + 0x02)

/**
 * @brief Clear a71x9 transmiter fifo, drop all untransmitted data.
 * @param Args is not used, it can be NULL.
 * @retval If clear success, it will return CY_EOK, otherwise
 *         return CY_ERROR.
 */
#define IOCTL_A71X9_CLEAR_TRANSMITER_FIFO                   (IOCTL_USER_START + 0x03)

/**
 * @brief According to the interrupt pend bits to execute the event callback function.
 * @param Args is not used, it can be NULL.
 * @retval If get interrupt pend bits success, it will return CY_EOK, otherwise
 *         return CY_ERROR.
 */
#define IOCTL_A71X9_INTERRUPT_HANDLING                      (IOCTL_USER_START + 0x04)

/**
 * @brief Set interrupt event callback.
 * @param Args type is pointer of `void (*)(a71x9_evt_t)`.
 * @retval If set callback success, it will return CY_EOK, otherwise
 *         return CY_ERROR.
 */
#define IOCTL_A71X9_SET_EVT_CALLBACK                        (IOCTL_USER_START + 0x05)

/**
 * @brief Set interrupt handler for a71x9 nirq pin.
 * @param Args type is pointer of `int32_t (*)(uint32_t, void *, uint32_t)`.
 * @retval If set handler success, it will return CY_EOK, otherwise
 *         return CY_ERROR.
 */
#define IOCTL_A71X9_SET_IRQ_HANDLER                         (IOCTL_USER_START + 0x06)

/*---------- type define ----------*/
typedef enum {
    A71X9_STATE_STANDBY,
    A71X9_STATE_RX,
    A71X9_STATE_TX,
} a71x9_state_t;

typedef enum {
    A71X9_INTERRUPT_TYPE_GIO1,
    A71X9_INTERRUPT_TYPE_GIO2,
} a71x9_interrupt_type_t;

typedef enum {
    A71X9_EVT_PACKET_RX,
    A71X9_EVT_PACKET_SENT,
    A71X9_EVT_SYNC_DETECTED,
    A71X9_EVT_PREAMBLE_DETECTED,
} a71x9_evt_t;

typedef struct {
    /**
     * @brief Initialize the board support package code(some peripherals, 
     *         e.g. spi, cs gpio and sdn gpio)
     * @retval Return the initialize result, success is true, failure is false
     */
    bool (*init)(void);
    /**
     * @brief Deinitialize the board support package code(some peripherals,
     *         e.g. spi, cs gpio and sdn gpio)
     * @retval None
     */
    void (*deinit)(void);
    /**
     * @brief Select the device for communication
     * @param select If true, select the device for communication,
     *               otherwise, unselect
     * @retval None
     */
    void (*select)(bool select);
    void (*set_byte)(uint8_t byte);
    uint8_t (*get_byte)(void);
    /**
     * @brief A71x9 event callback
     * @param evt reference @a71x9_evt_t.
     * @retval None
     */
    void (*evt_cb)(a71x9_evt_t evt);
    /**
     * @brief A71x9 interrupt handler for NIRQ pin.
     * @param irq irq handler pointer.
     * @param args not used.
     * @param length not used.
     * @retval CY_EOK.
     */
    int32_t (*irq_handler)(uint32_t irq, void *args, uint32_t length);
} a71x9_ops_t;

typedef struct {
    const uint8_t *reg_data;
    const uint8_t *pagea_data;
    const uint8_t *pageb_data;
} a71x9_configure_t;

typedef struct {
    uint8_t sync[8];
    a71x9_state_t state;
    a71x9_configure_t configure;
    a71x9_ops_t ops;
} a71x9_describe_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/

#ifdef __cplusplus
}
#endif
#endif /* __A71X9_H */
