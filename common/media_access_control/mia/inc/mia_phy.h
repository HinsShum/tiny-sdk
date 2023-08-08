/**
 * @file media_access_control\mia\inc\mia_phy.h
 *
 * Copyright (C) 2023
 *
 * mia_phy.h is free software: you can redistribute it and/or modify
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
#ifndef __MIA_PHY_H
#define __MIA_PHY_H

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
struct mia_phy_ops {
    bool (*get_bit)(void);
    bool (*set_bit)(bool bit);
    void (*monitor_start_bit)(bool enable);
};

typedef struct mia_phy *mia_phy_t;

typedef enum {
    MIA_PHY_EX_SEND_FAULT = -3,
    MIA_PHY_EX_RECV_FAULT = -2,
    MIA_PHY_EX_BUS_FAULT = -1,
    MIA_PHY_EX_NONE = 0,
    MIA_PHY_EX_RECV_OK = 1,
    MIA_PHY_EX_SEND_OK = 2,
} mia_phy_expection_t;

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
extern void mia_phy_set_recv_buf(mia_phy_t phy, uint8_t *pbuf, uint16_t capacity);
extern uint8_t *mia_phy_get_recv_buf(mia_phy_t phy, uint16_t *length);
extern void mia_phy_start_recving(mia_phy_t phy);
extern void mia_phy_start_sending(mia_phy_t phy, uint8_t *pbuf, uint16_t length);
extern mia_phy_expection_t mia_phy_recv_polling(mia_phy_t phy);
extern mia_phy_expection_t mia_phy_send_polling(mia_phy_t phy);
extern mia_phy_expection_t mia_phy_monitor_polling(mia_phy_t phy);
extern mia_phy_t mia_phy_new(uint16_t baudrate, struct mia_phy_ops *ops);
extern void mia_phy_delete(mia_phy_t phy);

#ifdef __cplusplus
}
#endif
#endif /* __MIA_PHY_H */
