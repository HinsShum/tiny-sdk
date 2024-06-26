/**
 * @file drv\at24cxx.c
 *
 * Copyright (C) 2021
 *
 * at24cxx.c is free software: you can redistribute it and/or modify
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
#include "at24cxx.h"
#include "i2c_bus.h"
#include "driver.h"
#include "errorno.h"
#include "options.h"
#include <string.h>

/*---------- macro ----------*/
#define TAG                                         "AT24Cxx"

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
static int32_t at24cxx_open(driver_t **pdrv);
static void at24cxx_close(driver_t **pdrv);
static int32_t at24cxx_write(driver_t **pdrv, void *buf, uint32_t offset, uint32_t length);
static int32_t at24cxx_read(driver_t **pdrv, void *buf, uint32_t offset, uint32_t length);
static int32_t at24cxx_ioctl(driver_t **pdrv, uint32_t cmd, void *args);

/* private ioctl functions
 */
static int32_t _ioctl_erase_block(at24cxx_describe_t *pdesc, void *args);
static int32_t _ioctl_erase_chip(at24cxx_describe_t *pdesc, void *args);
static int32_t _ioctl_check_addr_is_block_start(at24cxx_describe_t *pdesc, void *args);
static int32_t _ioctl_get_info(at24cxx_describe_t *pdesc, void *args);
static int32_t _ioctl_set_callback(at24cxx_describe_t *pdesc, void *args);
static int32_t _ioctl_set_event_callback(at24cxx_describe_t *pdesc, void *args);
static int32_t _ioctl_power_on(at24cxx_describe_t *pdesc, void *args);
static int32_t _ioctl_power_off(at24cxx_describe_t *pdesc, void *args);

/*---------- type define ----------*/
typedef int32_t (*ioctl_cb_func_t)(at24cxx_describe_t *pdesc, void *args);
typedef struct {
    uint32_t ioctl_cmd;
    ioctl_cb_func_t cb;
} ioctl_cb_t;

/*---------- variable ----------*/
DRIVER_DEFINED(at24cxx, at24cxx_open, at24cxx_close, at24cxx_write, at24cxx_read, at24cxx_ioctl, NULL);
const static ioctl_cb_t ioctl_cb_array[] = {
    {IOCTL_DEVICE_POWER_ON, _ioctl_power_on},
    {IOCTL_DEVICE_POWER_OFF, _ioctl_power_off},
    {IOCTL_FLASH_ERASE_BLOCK, _ioctl_erase_block},
    {IOCTL_FLASH_ERASE_CHIP, _ioctl_erase_chip},
    {IOCTL_FLASH_CHECK_ADDR_IS_BLOCK_START, _ioctl_check_addr_is_block_start},
    {IOCTL_FLASH_GET_INFO, _ioctl_get_info},
    {IOCTL_FLASH_SET_CALLBACK, _ioctl_set_callback},
    {IOCTL_AT24CXX_SET_EVENT_CALLBACK, _ioctl_set_event_callback},
};

/*---------- function ----------*/
static int32_t at24cxx_open(driver_t **pdrv)
{
    at24cxx_describe_t *pdesc = NULL;
    int32_t retval = CY_E_WRONG_ARGS;
    void *bus = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do {
        if(!pdesc) {
            xlog_tag_error(TAG, "driver has no describe field\n");
            break;
        }
        if(!pdesc->blk_buf) {
            xlog_tag_error(TAG, "Block buffer is NULL\n");
            break;
        }
        retval = CY_EOK;
        if(pdesc->ops.init) {
            if(!pdesc->ops.init()) {
                xlog_tag_error(TAG, "initialize failed\n");
                retval = CY_ERROR;
                break;
            }
        }
        /* bind to i2c bus */
        if(NULL == (bus = device_open(pdesc->bus_name))) {
            xlog_tag_error(TAG, "bind i2c bus failed\n");
            if(pdesc->ops.deinit) {
                pdesc->ops.deinit();
            }
            retval = CY_ERROR;
            break;
        }
        pdesc->bus = bus;
    } while(0);

    return retval;
}

static void at24cxx_close(driver_t **pdrv)
{
    at24cxx_describe_t *pdesc = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    if(pdesc) {
        if(pdesc->bus) {
            device_close(pdesc->bus);
            pdesc->bus = NULL;
        }
        if(pdesc->ops.deinit) {
            pdesc->ops.deinit();
        }
    }
}

static void inline _do_callback(at24cxx_describe_t *pdesc)
{
    if(pdesc->ops.cb) {
        pdesc->ops.cb();
    }
}

static void inline _do_on_event(at24cxx_describe_t *pdesc, struct at24cxx_event *evt)
{
    if(pdesc->ops.on_event && evt->type != AT24CXX_EVT_NONE) {
        pdesc->ops.on_event(evt);
    }
}

static void inline _do_write_cycle_time(at24cxx_describe_t *pdesc)
{
    if(pdesc->ops.write_cycle_time) {
        pdesc->ops.write_cycle_time();
    }
}

static void inline _do_write_protect_set(at24cxx_describe_t *pdesc, bool protect)
{
    if(pdesc->ops.write_protect_set) {
        pdesc->ops.write_protect_set(protect);
    }
}

static bool _read_back_and_check_write_bytes(at24cxx_describe_t *pdesc, i2c_bus_msg_t *pw)
{
    bool err = false;
    int32_t result = CY_ERROR;
    i2c_bus_msg_t r = {
        .type = I2C_BUS_TYPE_RANDOM_READ,
        .dev_addr = pw->dev_addr,
        .mem_addr = pw->mem_addr,
        .mem_addr_counts = pw->mem_addr_counts,
        .buf = pdesc->blk_buf,
        .len = pw->len
    };

    _do_write_cycle_time(pdesc);
    /* read back */
    device_ioctl(pdesc->bus, IOCTL_I2C_BUS_LOCK, NULL);
    result = device_read(pdesc->bus, &r, 0, sizeof(r));
    device_ioctl(pdesc->bus, IOCTL_I2C_BUS_UNLOCK, NULL);
    if(result == CY_EOK) {
        /* check bytes */
        if(memcmp(r.buf, pw->buf, r.len) == 0) {
            err = true;
        }
    }

    return err;
}

static bool _read_back_and_check_erase_bytes(at24cxx_describe_t *pdesc, i2c_bus_msg_t *pw)
{
    bool err = false;
    int32_t result = CY_ERROR;
    i2c_bus_msg_t r = {
        .type = I2C_BUS_TYPE_RANDOM_READ,
        .dev_addr = pw->dev_addr,
        .mem_addr = pw->mem_addr,
        .mem_addr_counts = pw->mem_addr_counts,
        .buf = pdesc->blk_buf,
        .len = pw->len
    };

    _do_write_cycle_time(pdesc);
    /* read back */
    device_ioctl(pdesc->bus, IOCTL_I2C_BUS_LOCK, NULL);
    result = device_read(pdesc->bus, &r, 0, sizeof(r));
    device_ioctl(pdesc->bus, IOCTL_I2C_BUS_UNLOCK, NULL);
    if(result == CY_EOK) {
        /* check bytes */
        err = true;
        for(uint32_t i = 0; i < r.len; ++i) {
            if(r.buf[i] != 0xFF) {
                err = false;
                break;
            }
        }
    }

    return err;
}

static int32_t at24cxx_write(driver_t **pdrv, void *buf, uint32_t offset, uint32_t length)
{
    i2c_bus_msg_t msg = {0};
    uint32_t actual_len = 0;
    at24cxx_describe_t *pdesc = NULL;
    int32_t result = CY_EOK;
    uint8_t memory_addr[2] = {0};
    uint32_t address = 0;
    struct at24cxx_event evt = {
        .type = AT24CXX_EVT_NONE
    };

    assert(pdrv);
    assert(buf);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do {
        if(!pdesc) {
            xlog_tag_error(TAG, "driver has no describe field\n");
            break;
        }
        if(!pdesc->bus) {
            xlog_tag_error(TAG, "not bind to i2c bus\n");
            break;
        }
        msg.type = I2C_BUS_TYPE_WRITE;
        msg.dev_addr = pdesc->address;
        msg.mem_addr = memory_addr;
        msg.mem_addr_counts = pdesc->mem_addr_counts;
        _do_write_protect_set(pdesc, false);
        address = pdesc->info.start + offset;
        if(address >= pdesc->info.end) {
            xlog_tag_error(TAG, "write address is overflow\n");
            length = 0;
        } else if((address + length) > pdesc->info.end) {
            length = pdesc->info.end - address;
            xlog_tag_warn(TAG, "write address plus length is overflow, it only can write %d bytes\n", length);
        }
        while(actual_len < length) {
            if(pdesc->mem_addr_counts == 1) {
                memory_addr[0] = address & 0xFF;
            } else {
                memory_addr[0] = (address >> 8) & 0xFF;
                memory_addr[1] = address & 0xFF;
            }
            msg.buf = (buf + actual_len);
            if(((address + (length - actual_len)) & (~(pdesc->info.block_size - 1))) !=
               (address & (~(pdesc->info.block_size - 1)))) {
                msg.len = ((address + pdesc->info.block_size) & (~(pdesc->info.block_size - 1))) - address;
            } else {
                msg.len = length - actual_len;
            }
            device_ioctl(pdesc->bus, IOCTL_I2C_BUS_LOCK, NULL);
            result = device_write(pdesc->bus, &msg, 0, sizeof(msg));
            device_ioctl(pdesc->bus, IOCTL_I2C_BUS_UNLOCK, NULL);
            _do_callback(pdesc);
            if(CY_EOK != result) {
                evt.type = AT24CXX_EVT_WRITE_FAILURE;
                evt.offset = address - pdesc->info.start;
                xlog_tag_error(TAG, "write failed\n");
                break;
            }
            /* read back and check bytes*/
            if(_read_back_and_check_write_bytes(pdesc, &msg) != true) {
                evt.type = AT24CXX_EVT_WRITE_FAILURE;
                evt.offset = address - pdesc->info.start;
                xlog_tag_error(TAG, "check bytes error, write failure\n");
                break;
            }
            actual_len += msg.len;
            address += msg.len;
        }
    } while(0);
    _do_write_protect_set(pdesc, true);
    _do_on_event(pdesc, &evt);

    return (int32_t)actual_len;
}

static int32_t at24cxx_read(driver_t **pdrv, void *buf, uint32_t offset, uint32_t length)
{
    at24cxx_describe_t *pdesc = NULL;
    uint32_t actual_len = 0;
    i2c_bus_msg_t msg = {0};
    uint8_t memory_addr[2] = {0};
    uint32_t address = 0;
    struct at24cxx_event evt = {
        .type = AT24CXX_EVT_NONE,
        .offset = offset
    };

    assert(pdrv);
    assert(buf);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do {
        if(!pdesc) {
            xlog_tag_error(TAG, "driver has no describe field\n");
            break;
        }
        if(!pdesc->bus) {
            xlog_tag_error(TAG, "not bind to i2c bus\n");
            break;
        }
        if(!length) {
            xlog_tag_error(TAG, "read length is zero\n");
            break;
        }
        msg.type = I2C_BUS_TYPE_RANDOM_READ;
        msg.dev_addr = pdesc->address;
        msg.mem_addr = memory_addr;
        msg.mem_addr_counts = pdesc->mem_addr_counts;
        address = pdesc->info.start + offset;
        if(address >= pdesc->info.end) {
            xlog_tag_error(TAG, "read address is overflow\n");
            length = 0;
            break;
        } else if((address + length) > pdesc->info.end) {
            length = pdesc->info.end - address;
            xlog_tag_warn(TAG, "read address plus length is overflow, it only can read %d bytes\n", length);
        }
        if(pdesc->mem_addr_counts == 1) {
            memory_addr[0] = address & 0xFF;
        } else {
            memory_addr[0] = (address >> 8) & 0xFF;
            memory_addr[1] = address & 0xFF;
        }
        msg.buf = buf;
        msg.len = length;
        _do_callback(pdesc);
        evt.type = AT24CXX_EVT_READ_FAILURE;
        device_ioctl(pdesc->bus, IOCTL_I2C_BUS_LOCK, NULL);
        if(CY_EOK == device_read(pdesc->bus, &msg, 0, sizeof(msg))) {
            evt.type = AT24CXX_EVT_NONE;
            actual_len = length;
        }
        device_ioctl(pdesc->bus, IOCTL_I2C_BUS_UNLOCK, NULL);
    } while(0);
    _do_on_event(pdesc, &evt);

    return (int32_t)actual_len;
}

static int32_t _ioctl_erase_block(at24cxx_describe_t *pdesc, void *args)
{
    int32_t retval = CY_E_WRONG_ARGS;
    uint32_t *poffset = (uint32_t *)args;
    uint32_t addr = 0;
    i2c_bus_msg_t msg = {0};
    uint8_t memory_addr[2] = {0};
    struct at24cxx_event evt = {
        .type = AT24CXX_EVT_NONE
    };

    do {
        if(!args) {
            xlog_tag_error(TAG, "Args is NULL, erase block function must specify the erase address\n");
            break;
        }
        memset(pdesc->blk_buf, 0xFF, pdesc->info.block_size);
        addr = ((*poffset + pdesc->info.start) / pdesc->info.block_size) * pdesc->info.block_size;
        if(pdesc->mem_addr_counts == 1) {
            memory_addr[0] = addr & 0xFF;
        } else {
            memory_addr[0] = (addr >> 8) & 0xFF;
            memory_addr[1] = addr & 0xFF;
        }
        msg.type = I2C_BUS_TYPE_WRITE;
        msg.dev_addr = pdesc->address;
        msg.mem_addr = memory_addr;
        msg.mem_addr_counts = pdesc->mem_addr_counts;
        msg.buf = pdesc->blk_buf;
        msg.len = pdesc->info.block_size;
        _do_write_protect_set(pdesc, false);
        device_ioctl(pdesc->bus, IOCTL_I2C_BUS_LOCK, NULL);
        retval = device_write(pdesc->bus, &msg, 0, sizeof(msg));
        device_ioctl(pdesc->bus, IOCTL_I2C_BUS_UNLOCK, NULL);
        _do_callback(pdesc);
        if(CY_EOK != retval) {
            evt.type = AT24CXX_EVT_ERASE_FAILURE;
            evt.offset = addr - pdesc->info.start;
            xlog_tag_error(TAG, "erase address(%08X) failed\n", addr);
            break;
        }
        /* read back and check bytes */
        if(_read_back_and_check_erase_bytes(pdesc, &msg) != true) {
            evt.type = AT24CXX_EVT_ERASE_FAILURE;
            evt.offset = addr - pdesc->info.start;
            retval = CY_ERROR;
            xlog_tag_error(TAG, "check bytes error, erase address(%08X) failure\n", addr);
            break;
        }
        retval = (int32_t)pdesc->info.block_size;
        xlog_tag_info(TAG, "Erase address(%08X) block size: %dbytes\n", addr, pdesc->info.block_size);
    } while(0);
    _do_write_protect_set(pdesc, true);
    _do_on_event(pdesc, &evt);

    return retval;
}

static int32_t _ioctl_erase_chip(at24cxx_describe_t *pdesc, void *args)
{
    uint32_t addr = pdesc->info.start;
    int32_t retval = CY_ERROR;
    uint8_t buf[128] = {0};
    i2c_bus_msg_t msg = {0};
    uint8_t memory_addr[2] = {0};
    struct at24cxx_event evt = {
        .type = AT24CXX_EVT_NONE
    };

    _do_write_protect_set(pdesc, false);
    memset(buf, 0xFF, ARRAY_SIZE(buf));
    while(addr < pdesc->info.end) {
        if(pdesc->mem_addr_counts == 1) {
            memory_addr[0] = addr & 0xFF;
        } else {
            memory_addr[0] = (addr >> 8) & 0xFF;
            memory_addr[1] = addr & 0xFF;
        }
        msg.type = I2C_BUS_TYPE_WRITE;
        msg.dev_addr = pdesc->address;
        msg.mem_addr = memory_addr;
        msg.mem_addr_counts = pdesc->mem_addr_counts;
        msg.buf = buf;
        msg.len = pdesc->info.block_size;
        device_ioctl(pdesc->bus, IOCTL_I2C_BUS_LOCK, NULL);
        retval = device_write(pdesc->bus, &msg, 0, sizeof(msg));
        device_ioctl(pdesc->bus, IOCTL_I2C_BUS_UNLOCK, NULL);
        _do_callback(pdesc);
        if(CY_EOK != retval) {
            evt.type = AT24CXX_EVT_ERASE_FAILURE;
            evt.offset = addr - pdesc->info.start;
            xlog_tag_error(TAG, "erase chip failed, %08X address occur error\n", addr);
            break;
        }
        /* read back and check bytes */
        if(_read_back_and_check_erase_bytes(pdesc, &msg) != true) {
            evt.type = AT24CXX_EVT_ERASE_FAILURE;
            evt.offset = addr - pdesc->info.start;
            retval = CY_ERROR;
            xlog_tag_error(TAG, "check bytes error, erase address(%08X) failure\n", addr);
            break;
        }
        xlog_tag_info(TAG, "Erase chip, current address: %08X\n", addr);
        addr += msg.len;
    }
    _do_write_protect_set(pdesc, true);
    _do_on_event(pdesc, &evt);
    if(retval == CY_EOK) {
        retval = (int32_t)(pdesc->info.end - pdesc->info.start);
    }

    return retval;
}

static int32_t _ioctl_check_addr_is_block_start(at24cxx_describe_t *pdesc, void *args)
{
    int32_t retval = CY_E_WRONG_ARGS;
    uint32_t *poffset = (uint32_t *)args;

    do {
        if(!args) {
            xlog_tag_error(TAG, "Args is NULL, can not check the addr\n");
            break;
        }
        if(((*poffset + pdesc->info.start) % pdesc->info.block_size) == 0) {
            retval = CY_EOK;
        } else {
            retval = CY_ERROR;
        }
    } while(0);

    return retval;
}

static int32_t _ioctl_get_info(at24cxx_describe_t *pdesc, void *args)
{
    int32_t retval = CY_E_WRONG_ARGS;
    flash_info_t *pinfo = (flash_info_t *)args;

    do {
        if(!args) {
            xlog_tag_error(TAG, "Args is NULL, no memory to store at24cxx information\n");
            break;
        }
        pinfo->start = pdesc->info.start;
        pinfo->end = pdesc->info.end;
        pinfo->block_size = pdesc->info.block_size;
        retval = CY_EOK;
    } while(0);

    return retval;
}

static int32_t _ioctl_set_callback(at24cxx_describe_t *pdesc, void *args)
{
    int32_t retval = CY_E_WRONG_ARGS;
    void (*cb)(void) = (void (*)(void))args;

    do {
        if(!args) {
            xlog_tag_error(TAG, "Args is NULL, no callback to bind the at24cxx device\n");
            break;
        }
        pdesc->ops.cb = cb;
        retval = CY_EOK;
    } while(0);

    return retval;
}

static int32_t _ioctl_set_event_callback(at24cxx_describe_t *pdesc, void *args)
{
    int32_t retval = CY_E_WRONG_ARGS;
    void (*on_event)(struct at24cxx_event *) = (void (*)(struct at24cxx_event *))args;

    do {
        if(!args) {
            xlog_tag_error(TAG, "Args is NULL, no event callback to bind the at24cxx device\n");
            break;
        }
        pdesc->ops.on_event = on_event;
        retval = CY_EOK;
    } while(0);

    return retval;
}

static int32_t _ioctl_power_on(at24cxx_describe_t *pdesc, void *args)
{
    if(pdesc->ops.power) {
        pdesc->ops.power(true);
    }

    return CY_EOK;
}

static int32_t _ioctl_power_off(at24cxx_describe_t *pdesc, void *args)
{
    if(pdesc->ops.power) {
        pdesc->ops.power(false);
    }

    return CY_EOK;
}

static ioctl_cb_func_t _ioctl_cb_func_find(uint32_t ioctl_cmd)
{
    ioctl_cb_func_t cb = NULL;

    for(uint32_t i = 0; i < ARRAY_SIZE(ioctl_cb_array); ++i) {
        if(ioctl_cb_array[i].ioctl_cmd == ioctl_cmd) {
            cb = ioctl_cb_array[i].cb;
            break;
        }
    }

    return cb;
}

static int32_t at24cxx_ioctl(driver_t **pdrv, uint32_t cmd, void *args)
{
    at24cxx_describe_t *pdesc = NULL;
    int32_t retval = CY_E_WRONG_ARGS;
    ioctl_cb_func_t cb = NULL;

    assert(pdrv);
    pdesc = container_of(pdrv, device_t, pdrv)->pdesc;
    do {
        if(!pdesc) {
            xlog_tag_error(TAG, "has no describe field\n");
            break;
        }
        if(NULL == (cb = _ioctl_cb_func_find(cmd))) {
            xlog_tag_error(TAG, "not support this command(%08X)\n", cmd);
            break;
        }
        retval = cb(pdesc, args);
    } while(0);

    return retval;
}
