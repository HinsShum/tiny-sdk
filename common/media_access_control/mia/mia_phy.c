/**
 * @file media_access_control\mia\mia_phy.c
 *
 * Copyright (C) 2023
 *
 * mia_phy.c is free software: you can redistribute it and/or modify
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
#include "mia_phy.h"
#include "options.h"
#include <string.h>

/*---------- macro ----------*/
/*---------- type define ----------*/
struct mia_phy {
    /* phy variables */
    bool former_bit;
    uint8_t bit_count;
    uint16_t tick_count;
    uint16_t us_per_tick;
    enum {
        MIA_PHY_IDLE,
        MIA_PHY_SENDING,
        MIA_PHY_RECVING,
        MIA_PHY_ERROR,
    } phy;
    struct {
        uint8_t *pbuf;
        uint16_t capacity;
        uint16_t offset;
    } recv;
    struct {
        uint8_t *pbuf;
        uint16_t length;
        uint16_t offset;
    } trans;
    /* phy operate functions */
    struct mia_phy_ops ops;
};

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/*---------- variable ----------*/
/*---------- function ----------*/
static bool _get_bit(void)
{
    return true;
}

static bool _set_bit(bool bit)
{
    return true;
}

static void _monitor_start_bit(bool enable)
{
}

void mia_phy_set_recv_buf(mia_phy_t phy, uint8_t *pbuf, uint16_t capacity)
{
    phy->recv.pbuf = pbuf;
    phy->recv.capacity = capacity;
    memset(phy->recv.pbuf, 0, phy->recv.capacity);
}

uint8_t *mia_phy_get_recv_buf(mia_phy_t phy, uint16_t *length)
{
    if(length) {
        *length = (phy->recv.offset < phy->recv.capacity ?
                phy->recv.offset : phy->recv.capacity);
    }

    return phy->recv.pbuf;
}

void mia_phy_start_recving(mia_phy_t phy)
{
    phy->former_bit = true;
    phy->bit_count = 0;
    phy->tick_count = 0;
    phy->recv.offset = 0;
    phy->phy = MIA_PHY_RECVING;
    phy->ops.monitor_start_bit(false);
}

void mia_phy_start_sending(mia_phy_t phy, uint8_t *pbuf, uint16_t length)
{
    phy->former_bit = false;
    phy->bit_count = 0;
    phy->tick_count = 0;
    phy->trans.pbuf = pbuf;
    phy->trans.length = length;
    phy->trans.offset = 0;
    phy->phy = MIA_PHY_SENDING;
    phy->ops.monitor_start_bit(false);
}

mia_phy_expection_t mia_phy_recv_polling(mia_phy_t phy)
{
    mia_phy_expection_t err = MIA_PHY_EX_NONE;
    bool bit = false;
    bool former_bit = phy->former_bit;

    do {
        if(phy->phy != MIA_PHY_RECVING) {
            break;
        }
        bit = phy->ops.get_bit();
        phy->former_bit = bit;
        phy->tick_count++;
        if(bit != former_bit) {
            /* edge detected */
            if(!phy->bit_count) {
                if(phy->tick_count > 2) {
                    /* start bit */
                    phy->tick_count = 0;
                    phy->bit_count++;
                }
            } else {
                /* data field */
                if(phy->tick_count > 6) {
                    if(phy->recv.offset < phy->recv.capacity) {
                        phy->recv.pbuf[phy->recv.offset] |= (bit << (8 - phy->bit_count));
                    }
                    if(phy->bit_count == 8) {
                        phy->bit_count = 1;
                        phy->recv.offset++;
                    } else {
                        phy->bit_count++;
                    }
                    phy->tick_count = 0;
                }
            }
            break;
        }
        /* level stable */
        if(!phy->bit_count) {
            if(phy->tick_count == 8) {
                /* too long start bit */
                phy->phy = MIA_PHY_ERROR;
            }
        } else {
            if(phy->tick_count == 10) {
                if(phy->bit_count != 1 || !bit) {
                    /* too long delay before edge or wrong level of 1st stop bit */
                    phy->phy = MIA_PHY_ERROR;
                }
            } else if(phy->tick_count == 18) {
                /* 2nd stop bit detected */
                phy->phy = MIA_PHY_IDLE;
                phy->tick_count = 0;
                phy->ops.monitor_start_bit(true);
                err = (phy->recv.offset ? MIA_PHY_EX_RECV_OK : MIA_PHY_EX_RECV_FAULT);
            }
        }
    } while(0);
    if(phy->phy == MIA_PHY_ERROR) {
        phy->phy = MIA_PHY_IDLE;
        phy->tick_count = 0;
        phy->ops.monitor_start_bit(true);
        err = MIA_PHY_EX_RECV_FAULT;
    }

    return err;
}

mia_phy_expection_t mia_phy_send_polling(mia_phy_t phy)
{
    mia_phy_expection_t err = MIA_PHY_EX_NONE;
    bool bit = false;

    do {
        if(phy->phy != MIA_PHY_SENDING) {
            break;
        }
        if((phy->tick_count & 0x03) == 0x02) {
            if(phy->ops.get_bit() != phy->former_bit) {
                phy->ops.set_bit(true);
                phy->tick_count = 0;
                phy->phy = MIA_PHY_IDLE;
                err = MIA_PHY_EX_SEND_FAULT;
                break;
            }
        }
        if(!(phy->tick_count & 0x03)) {
            if(phy->tick_count == 0) {              /*<< start bit low */
                phy->former_bit = false;
            } else if(phy->tick_count == 4) {       /*<< start bit high */
                phy->former_bit = true;
            } else if(phy->tick_count <= 72) {
                if(phy->tick_count == 72) {         /*<< end of last bit for byte */
                    phy->trans.offset++;
                    if(phy->trans.offset < phy->trans.length) {
                        /* send next byte */
                        phy->tick_count = 8;
                    } else {
                        /* 1st stop bit */
                        phy->former_bit = true;
                    }
                }
                if(phy->tick_count < 72) {
                    bit = (bool)((phy->trans.pbuf[phy->trans.offset] >> (7 - phy->bit_count)) & 0x01);
                    /* data bit */
                    if(!(phy->tick_count & 0x07)) {
                        phy->former_bit = !bit;
                    } else if(!((phy->tick_count - 4) & 0x07)) {
                        phy->former_bit = bit;
                        phy->bit_count = (phy->bit_count + 1) & 0x07;
                    }
                }
            } else if(phy->tick_count == 88) {      /*<< end of 2nd stop bits */
                phy->phy = MIA_PHY_IDLE;
                phy->tick_count = 0;
                phy->ops.monitor_start_bit(true);
                err = MIA_PHY_EX_SEND_OK;
                break;
            }
        }
        phy->ops.set_bit(phy->former_bit);
        phy->tick_count++;
    } while(0);

    return err;
}

mia_phy_expection_t mia_phy_monitor_polling(mia_phy_t phy)
{
    mia_phy_expection_t err = MIA_PHY_EX_NONE;

    do {
        if(phy->phy != MIA_PHY_IDLE) {
            break;
        }
        if(phy->ops.get_bit()) {
            phy->tick_count = 0;
            break;
        }
        if(phy->tick_count > ((1000UL * 500) / phy->us_per_tick)) {
            err = MIA_PHY_EX_BUS_FAULT;
        } else {
            phy->tick_count++;
        }
    } while(0);

    return err;
}

mia_phy_t mia_phy_new(uint16_t baudrate, struct mia_phy_ops *ops)
{
    mia_phy_t phy = __malloc(sizeof(struct mia_phy));

    if(phy) {
        memset(phy, 0, sizeof(*phy));
        phy->us_per_tick = (uint16_t)(1000000UL / (baudrate * 8));
        phy->ops.get_bit = (ops->get_bit ? ops->get_bit : _get_bit);
        phy->ops.set_bit = (ops->set_bit ? ops->set_bit : _set_bit);
        phy->ops.monitor_start_bit = (ops->monitor_start_bit ? ops->monitor_start_bit : _monitor_start_bit);
        phy->ops.monitor_start_bit(true);
    }

    return phy;
}

void mia_phy_delete(mia_phy_t phy)
{
    if(phy) {
        assert(phy->ops.monitor_start_bit);
        phy->ops.monitor_start_bit(false);
        __free(phy);
    }
}
