/**
 * @file components\tiny-sdk\drv\a71x9.c
 *
 * Copyright (C) 2023
 *
 * a71x9.c is free software: you can redistribute it and/or modify
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
#include "a71x9.h"
#include "driver.h"
#include "errorno.h"
#include "options.h"
#include <string.h>

/*---------- macro ----------*/
#undef BIT
#define BIT(n)                                      (1UL << (n))
#define TAG                                         "A71x9"

/*---------- type define ----------*/
typedef enum {
    /* regs */
    REG_SYSTEM_CLOCK = 0x00,
    REG_PLL1 = 0x01,
    REG_PLL2 = 0x02,
    REG_PLL3 = 0x03,
    REG_PLL4 = 0x04,
    REG_PLL5 = 0x05,
    REG_PLL6 = 0x06,
    REG_CRYSTAL = 0x07,
    REG_PAGEA = 0x08,
    REG_PAGEB = 0x09,
    REG_RX1 = 0x0A,
    REG_RX2 = 0x0B,
    REG_ADC = 0x0C,
    REG_PIN_CONTROL = 0x0D,
    REG_CALIBRATION = 0x0E,
    REG_MODE_CONTROL = 0x0F,
    /* pagea regs */
    REG_PAGEA_TX1 = 0x00,
    REG_PAGEA_WOR1 = 0x01,
    REG_PAGEA_WOR2 = 0x02,
    REG_PAGEA_RFI = 0x03,
    REG_PAGEA_PM = 0x04,
    REG_PAGEA_RTH = 0x05,
    REG_PAGEA_AGC1 = 0x06,
    REG_PAGEA_AGC2 = 0x07,
    REG_PAGEA_GPIO = 0x08,
    REG_PAGEA_CKO = 0x09,
    REG_PAGEA_VCB = 0x0A,
    REG_PAGEA_CHG1 = 0x0B,
    REG_PAGEA_CHG2 = 0x0C,
    REG_PAGEA_FIFO = 0x0D,
    REG_PAGEA_CODE = 0x0E,
    REG_PAGEA_WCAL = 0x0F,
    /* pageb regs */
    REG_PAGEB_TX2 = 0x00,
    REG_PAGEB_IF1 = 0x01,
    REG_PAGEB_IF2 = 0x02,
    REG_PAGEB_ACK = 0x03,
    REG_PAGEB_ART = 0x04,
} a7139_reg_t;

typedef enum {
    CMD_WRITE_REG = 0x00,
    CMD_READ_REG = 0x80,
    CMD_WRITE_ID = 0x20,
    CMD_READ_ID = 0xA0,
    CMD_WRITE_TX_FIFO = 0x40,
    CMD_READ_RX_FIFO = 0xC0,
    CMD_RESET_TX_FIFO = 0x60,
    CMD_RESET_RX_FIFO = 0xE0,
    CMD_RESET = 0x70,
    CMD_SLEEP_MODE = 0x10,
    CMD_IDLE_MODE = 0x12,
    CMD_STANDBY_MODE = 0x14,
    CMD_PLL_MODE = 0x16,
    CMD_RX_MODE = 0x18,
    CMD_TX_MODE = 0x1A,
    CMD_DEEP_SLEEP_MODE_TRI_STATE = 0x1C,
    CMD_DEEP_SLEEP_MODE_PULL_HIGH = 0x1F
} a7139_cmd_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
static int32_t a71x9_open(driver_t **pdrv);
static void a71x9_close(driver_t **pdrv);
static int32_t a71x9_write(driver_t **pdrv, void *pbuf, uint32_t addition, uint32_t length);
static int32_t a71x9_read(driver_t **pdrv, void *pbuf, uint32_t nouse, uint32_t length);
static int32_t a71x9_ioctl(driver_t **pdrv, uint32_t cmd, void *args);
static int32_t a71x9_irq_handler(driver_t **pdrv, uint32_t irq_handler, void *args, uint32_t length);
static int32_t _ioctl_reinitialize(a71x9_describe_t *pdesc, void *args);
static int32_t _ioctl_start_receiving(a71x9_describe_t *pdesc, void *args);
static int32_t _ioctl_clear_receive_fifo(a71x9_describe_t *pdesc, void *args);
static int32_t _ioctl_clear_transmit_fifo(a71x9_describe_t *pdesc, void *args);
static int32_t _ioctl_interrupt_handling(a71x9_describe_t *pdesc, void *args);
static int32_t _ioctl_set_evt_cb(a71x9_describe_t *pdesc, void *args);
static int32_t _ioctl_set_interrupt_handler(a71x9_describe_t *pdesc, void *args);

/*---------- variable ----------*/
DRIVER_DEFINED(a71x9, a71x9_open, a71x9_close, a71x9_write, a71x9_read, a71x9_ioctl, a71x9_irq_handler);
const static struct protocol_callback ioctl_cbs[] = {
    {IOCTL_A71X9_REINITIALIZE, _ioctl_reinitialize},
    {IOCTL_A71X9_START_RECEIVING, _ioctl_start_receiving},
    {IOCTL_A71X9_CLEAR_RECEIVER_FIFO, _ioctl_clear_receive_fifo},
    {IOCTL_A71X9_CLEAR_TRANSMITER_FIFO, _ioctl_clear_transmit_fifo},
    {IOCTL_A71X9_INTERRUPT_HANDLING, _ioctl_interrupt_handling},
    {IOCTL_A71X9_SET_EVT_CALLBACK, _ioctl_set_evt_cb},
    {IOCTL_A71X9_SET_IRQ_HANDLER, _ioctl_set_interrupt_handler},
};

/*---------- function ----------*/
static inline void _write_cmd(a71x9_describe_t *pdesc, uint8_t cmd)
{
    pdesc->ops.select(true);
    pdesc->ops.set_byte(cmd);
    pdesc->ops.select(false);
}

static inline void _write_reg(a71x9_describe_t *pdesc, uint8_t reg, uint16_t data)
{
    pdesc->ops.select(true);
    pdesc->ops.set_byte((reg & 0x0F) | CMD_WRITE_REG);
    pdesc->ops.set_byte((data >> 8) & 0xFF);
    pdesc->ops.set_byte(data & 0xFF);
    pdesc->ops.select(false);
}

static inline uint16_t _read_reg(a71x9_describe_t *pdesc, uint8_t reg)
{
    uint16_t data = 0;

    pdesc->ops.select(true);
    pdesc->ops.set_byte((reg & 0x0F) | CMD_READ_REG);
    data = ((uint16_t)pdesc->ops.get_byte()) << 8;
    data |= pdesc->ops.get_byte();
    pdesc->ops.select(false);

    return data;
}

static inline void _soft_reset(a71x9_describe_t *pdesc)
{
    _write_cmd(pdesc, CMD_RESET);
    __delay_ms(10);
}

static inline void _write_id(a71x9_describe_t *pdesc, uint8_t *id, uint8_t length)
{
    length = (length > 8 ? 8 : length);
    pdesc->ops.select(true);
    pdesc->ops.set_byte(CMD_WRITE_ID);
    for(uint8_t i = 0; i < length; ++i) {
        pdesc->ops.set_byte(id[i]);
    }
    pdesc->ops.select(false);
}

static inline uint8_t _read_id(a71x9_describe_t *pdesc, uint8_t *id, uint8_t length)
{
    length = (length > 8 ? 8 : length);
    pdesc->ops.select(true);
    pdesc->ops.set_byte(CMD_READ_ID);
    for(uint8_t i = 0; i < length; ++i) {
        id[i] = pdesc->ops.get_byte();
    }
    pdesc->ops.select(false);

    return length;
}

static inline uint16_t __get_default_reg_data(a71x9_describe_t *pdesc, uint8_t reg, const uint8_t *data)
{
    uint16_t reg_data = 0;
    const uint8_t *configure_data = data;

    while(configure_data[0] != 0x00) {
        if(configure_data[1] == reg) {
            reg_data = ((uint16_t)configure_data[2] << 8) | configure_data[3];
            break;
        }
        configure_data += configure_data[0] + 1;
    }

    return reg_data;
}

static bool _calibration(a71x9_describe_t *pdesc)
{
    bool err = false;
    uint16_t reg_crystal = 0;
    uint16_t reg_mode_control = 0;
    uint16_t reg_adc = 0;
    uint16_t reg_pll1 = 0;
    uint16_t reg_pll2 = 0;
    uint16_t reg = 0;
    const uint8_t *configure_data = pdesc->configure.reg_data;

    do {
        /* get default reg data */
        while(configure_data[0] != 0x00) {
            if(configure_data[1] == REG_CRYSTAL) {
                reg_crystal = ((uint16_t)configure_data[2] << 8) | configure_data[3];
            } else if(configure_data[1] == REG_MODE_CONTROL) {
                reg_mode_control = ((uint16_t)configure_data[2] << 8) | configure_data[3];
            } else if(configure_data[1] == REG_ADC) {
                reg_adc = ((uint16_t)configure_data[2] << 8) | configure_data[3];
            } else if(configure_data[1] == REG_PLL1) {
                reg_pll1 = ((uint16_t)configure_data[2] << 8) | configure_data[3];
            } else if(configure_data[1] == REG_PLL2) {
                reg_pll2 = ((uint16_t)configure_data[2] << 8) | configure_data[3];
            }
            configure_data += configure_data[0] + 1;
        }
        /* start calibration */
        _write_reg(pdesc, REG_MODE_CONTROL, reg_mode_control | 0x0802);
        while(!!(_read_reg(pdesc, REG_MODE_CONTROL) & 0x0802));
        /* check IF filter */
        reg = _read_reg(pdesc, REG_CALIBRATION);
        if(reg & BIT(4)) {
            xlog_tag_error(TAG, "FBCF failure\n");
            break;
        }
        /* check vcb */
        _write_reg(pdesc, REG_CRYSTAL, reg_crystal | ((uint16_t)REG_PAGEA_VCB << 12));
        reg = _read_reg(pdesc, REG_PAGEA);
        if(reg & BIT(4)) {
            xlog_tag_error(TAG, "VCCF failure\n");
            break;
        }
        /* RSSI Calibration procedure @STB state */
        _write_reg(pdesc, REG_ADC, 0x4C00);
        _write_reg(pdesc, REG_MODE_CONTROL, reg_mode_control | 0x1000);
        while(!!(_read_reg(pdesc, REG_MODE_CONTROL) & 0x1000));
        _write_reg(pdesc, REG_ADC, reg_adc);
        /* VCO Calibration procedure @STB state */
        _write_reg(pdesc, REG_PLL1, reg_pll1);
        _write_reg(pdesc, REG_PLL2, reg_pll2);
        _write_reg(pdesc, REG_MODE_CONTROL, reg_mode_control | 0x0004);
        while(!!(_read_reg(pdesc, REG_MODE_CONTROL) & 0x0004));
        reg = _read_reg(pdesc, REG_CALIBRATION);
        if(reg & BIT(8)) {
            xlog_tag_error(TAG, "VBCF failure\n");
            break;
        }
        err = true;
    } while(0);

    return err;
}

static bool _reinitialize(a71x9_describe_t *pdesc)
{
    bool err = false;
    const uint8_t *configure_data = pdesc->configure.reg_data;
    uint16_t crystal = 0;
    uint16_t system_clock = 0;
    uint8_t sync[8] = {0};
    uint8_t sync_bytes = 0;

    do {
        if(pdesc->configure.reg_data == NULL ||
                pdesc->configure.pagea_data == NULL ||
                pdesc->configure.pageb_data == NULL) {
            xlog_tag_error(TAG, "No configure data exit, initialize si446x faulure\n");
            break;
        }
        _soft_reset(pdesc);
        /* write regs */
        while(configure_data[0] != 0x00) {
            uint16_t data = ((uint16_t)configure_data[2] << 8) | configure_data[3];
            if(configure_data[1] == (uint8_t)REG_SYSTEM_CLOCK) {
                system_clock = data;
            } else if(configure_data[1] == (uint8_t)REG_CRYSTAL) {
                crystal = data;
            }
            _write_reg(pdesc, configure_data[1], data);
            configure_data += configure_data[0] + 1;
        }
        /* write pagea regs */
        configure_data = pdesc->configure.pagea_data;
        while(configure_data[0] != 0x00) {
            if(configure_data[1] == REG_PAGEA_CODE) {
                sync_bytes = ((((uint8_t)(!!(configure_data[2] & BIT(6))) << 1) | (!!(configure_data[3] & BIT(2)))) + 1) << 1;
            }
            crystal = (crystal & (~(0xF << 12))) | ((uint16_t)(configure_data[1] & 0x0F) << 12);
            _write_reg(pdesc, REG_CRYSTAL, crystal);
            _write_reg(pdesc, REG_PAGEA, ((uint16_t)configure_data[2] << 8) | configure_data[3]);
            configure_data += configure_data[0] + 1;
        }
        /* write pageb regs */
        configure_data = pdesc->configure.pageb_data;
        while(configure_data[0] != 0x00) {
            crystal = (crystal & (~(0x7 << 7))) | ((uint16_t)(configure_data[1] & 0x07) << 7);
            _write_reg(pdesc, REG_CRYSTAL, crystal);
            _write_reg(pdesc, REG_PAGEB, ((uint16_t)configure_data[2] << 8) | configure_data[3]);
            configure_data += configure_data[0] + 1;
        }
        /* check reg system clock */
        if(_read_reg(pdesc, REG_SYSTEM_CLOCK) != system_clock) {
            xlog_tag_error(TAG, "Write reg to a71x9 failure\n");
            break;
        }
        __delay_ms(1);
        /* write id */
        _write_id(pdesc, pdesc->sync, sync_bytes);
        _read_id(pdesc, sync, sync_bytes);
        if(memcmp(pdesc->sync, sync, sync_bytes) != 0) {
            xlog_tag_error(TAG, "Write sync to a71x9 failure\n");
            break;
        }
        /* calibration */
        if(_calibration(pdesc) != true) {
            break;
        }
        __delay_ms(1);
        /* enter standby */
        _write_cmd(pdesc, CMD_STANDBY_MODE);
        pdesc->state = A71X9_STATE_STANDBY;
        err = true;
    } while(0);

    return err;
}

static int32_t a71x9_open(driver_t **pdrv)
{
    int32_t err = CY_E_WRONG_ARGS;
    a71x9_describe_t *pdesc = NULL;

    do {
        assert(pdrv);
        pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
        assert(pdesc);
        if(!pdesc) {
            xlog_tag_error(TAG, "No desc found\n");
            break;
        }
        if(pdesc->ops.init) {
            if(pdesc->ops.init() != true) {
                xlog_tag_error(TAG, "Initialize low level failure\n");
                err = CY_ERROR;
                break;
            }
        }
        /* unselect a71x9 */
        pdesc->ops.select(false);
        /* initialize a71x9 */
        if(_reinitialize(pdesc) != true) {
            if(pdesc->ops.deinit) {
                pdesc->ops.deinit();
            }
            err = CY_ERROR;
            break;
        }
        err = CY_EOK;
    } while(0);

    return err;
}

static void a71x9_close(driver_t **pdrv)
{
    a71x9_describe_t *pdesc = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc && pdesc->ops.deinit) {
        pdesc->ops.deinit();
    }
}

static inline uint8_t _get_received_data(a71x9_describe_t *pdesc, uint8_t *pbuf, uint16_t length)
{
    uint8_t bytes = 0;

    _write_cmd(pdesc, CMD_RESET_RX_FIFO);
    /* read data from fifo */
    pdesc->ops.select(true);
    pdesc->ops.set_byte(CMD_READ_RX_FIFO);
    bytes = pdesc->ops.get_byte();
    bytes = (bytes > length ? length : bytes);
    for(uint8_t i = 0; i < bytes; ++i) {
        pbuf[i] = pdesc->ops.get_byte();
    }
    pdesc->ops.select(false);

    return bytes;
}

static int32_t a71x9_read(driver_t **pdrv, void *pbuf, uint32_t nouse, uint32_t length)
{
    int32_t err = CY_E_WRONG_ARGS;
    a71x9_describe_t *pdesc = NULL;

    do {
        assert(pdrv);
        pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
        if(!pdesc) {
            break;
        }
        if(!pbuf || !length) {
            break;
        }
        err = (int32_t)_get_received_data(pdesc, pbuf, (uint16_t)length);
        /* enter rx mode */
        _write_cmd(pdesc, CMD_RESET_RX_FIFO);
        _write_cmd(pdesc, CMD_RX_MODE);
    } while(0);

    return err;
}

static inline void _write_variable_length(a71x9_describe_t *pdesc, uint8_t *pbuf, uint16_t length)
{
    _write_cmd(pdesc, CMD_STANDBY_MODE);
    _write_cmd(pdesc, CMD_RESET_TX_FIFO);
    pdesc->ops.select(true);
    pdesc->ops.set_byte(CMD_WRITE_TX_FIFO);
    pdesc->ops.set_byte((uint8_t)length);
    for(uint16_t i = 0; i < length; ++i) {
        pdesc->ops.set_byte(pbuf[i]);
    }
    pdesc->ops.select(false);
    _write_cmd(pdesc, CMD_TX_MODE);
    pdesc->state = A71X9_STATE_TX;
}

static int32_t a71x9_write(driver_t **pdrv, void *pbuf, uint32_t addition, uint32_t length)
{
    int32_t err = CY_E_WRONG_ARGS;
    a71x9_describe_t *pdesc = NULL;

    do {
        assert(pdrv);
        pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
        if(!pdesc) {
            break;
        }
        if(!pbuf || !length) {
            break;
        }
        length = (length > 63 ? 63 : length);
        _write_variable_length(pdesc, pbuf, length);
        err = CY_EOK;
    } while(0);

    return err;
}

static int32_t _ioctl_reinitialize(a71x9_describe_t *pdesc, void *args)
{
    return (_reinitialize(pdesc) ? CY_EOK : CY_ERROR);
}

static int32_t _ioctl_start_receiving(a71x9_describe_t *pdesc, void *args)
{
    _write_cmd(pdesc, CMD_STANDBY_MODE);
    _write_cmd(pdesc, CMD_RESET_RX_FIFO);
    _write_cmd(pdesc, CMD_RX_MODE);
    pdesc->state = A71X9_STATE_RX;

    return CY_EOK;
}

static int32_t _ioctl_clear_receive_fifo(a71x9_describe_t *pdesc, void *args)
{
    _write_cmd(pdesc, CMD_RESET_RX_FIFO);

    return CY_EOK;
}

static int32_t _ioctl_clear_transmit_fifo(a71x9_describe_t *pdesc, void *args)
{
    _write_cmd(pdesc, CMD_RESET_TX_FIFO);

    return CY_EOK;
}

static inline void __tx_state_interrupt_handling(a71x9_describe_t *pdesc, uint8_t gios)
{
    switch(gios) {
        case 0x00:
            pdesc->ops.evt_cb(A71X9_EVT_PACKET_SENT);
            break;
        case 0x03:
            pdesc->ops.evt_cb(A71X9_EVT_PREAMBLE_DETECTED);
            break;
        default:
            break;
    }
}

static inline void __rx_state_interrupt_handling(a71x9_describe_t *pdesc, uint8_t gios)
{
    switch(gios) {
        case 0x00:
            pdesc->ops.evt_cb(A71X9_EVT_PACKET_RX);
            break;
        case 0x01:
            pdesc->ops.evt_cb(A71X9_EVT_SYNC_DETECTED);
            break;
        case 0x03:
            pdesc->ops.evt_cb(A71X9_EVT_PREAMBLE_DETECTED);
            break;
        default:
            break;
    }
}

static int32_t _ioctl_interrupt_handling(a71x9_describe_t *pdesc, void *args)
{
    int32_t err = CY_E_WRONG_ARGS;
    uint16_t reg_data = __get_default_reg_data(pdesc, REG_PAGEA_GPIO, pdesc->configure.pagea_data);
    a71x9_interrupt_type_t type = 0;
    uint8_t gios = 0;       /*<< gpio pin function select */

    do {
        if(!args) {
            break;
        }
        type = *(a71x9_interrupt_type_t *)args;
        if(type == A71X9_INTERRUPT_TYPE_GIO1) {
            gios = (uint8_t)(reg_data >> 2) & 0x0F;
        } else if(type == A71X9_INTERRUPT_TYPE_GIO2) {
            gios = (uint8_t)(reg_data >> 8) & 0x0F;
        } else {
            break;
        }
        if(pdesc->state == A71X9_STATE_TX) {
            __tx_state_interrupt_handling(pdesc, gios);
        } else if(pdesc->state == A71X9_STATE_RX) {
            __rx_state_interrupt_handling(pdesc, gios);
        }
        err = CY_EOK;
    } while(0);

    return err;
}

static void __evt_cb_default(a71x9_evt_t evt)
{
}

static int32_t _ioctl_set_evt_cb(a71x9_describe_t *pdesc, void *args)
{
    void (*cb)(a71x9_evt_t) = (void (*)(a71x9_evt_t)) args;

    pdesc->ops.evt_cb = (cb ? cb : __evt_cb_default);

    return CY_EOK;
}

static int32_t _ioctl_set_interrupt_handler(a71x9_describe_t *pdesc, void *args)
{
    int32_t (*cb)(uint32_t, void *, uint32_t) = (int32_t (*)(uint32_t, void *, uint32_t))args;

    pdesc->ops.irq_handler = cb;

    return CY_EOK;
}

static int32_t a71x9_ioctl(driver_t **pdrv, uint32_t cmd, void *args)
{
    int32_t err = CY_E_WRONG_ARGS;
    a71x9_describe_t *pdesc = NULL;
    int32_t (*cb)(a71x9_describe_t *pdesc, void *args) = NULL;

    do {
        assert(pdrv);
        pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
        if(!pdesc) {
            break;
        }
        cb = protocol_callback_find(cmd, (void *)ioctl_cbs, ARRAY_SIZE(ioctl_cbs));
        if(!cb) {
            break;
        }
        err = cb(pdesc, args);
    } while(0);

    return err;
}

static int32_t a71x9_irq_handler(driver_t **pdrv, uint32_t irq_handler, void *args, uint32_t length)
{
    a71x9_describe_t *pdesc = NULL;
    int32_t err = CY_E_WRONG_ARGS;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc && pdesc->ops.irq_handler) {
        err = pdesc->ops.irq_handler(irq_handler, args, length);
    }

    return err;
}
