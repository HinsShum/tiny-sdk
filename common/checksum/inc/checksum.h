/**
 * @file common\checksum\inc\checksum.h
 *
 * Copyright (C) 2022
 *
 * checksum.h is free software: you can redistribute it and/or modify
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
#ifndef __CHECKSUM_H
#define __CHECKSUM_H

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
/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/* crc16 */
extern uint16_t checksum_crc16_modbus(void *data, uint16_t len);
extern uint16_t checksum_crc16_xmodem(void *data, uint16_t len);
extern uint16_t checksum_crc16_maxim(void *data, uint16_t len);
extern uint16_t checksum_crc16_ibm(void *data, uint16_t len);
extern uint16_t checksum_crc16_ccitt(void *data, uint16_t len);
/* crc8 */
extern uint8_t checksum_crc8(void *data, uint16_t len);
extern uint8_t checksum_crc8_rohc(void *data, uint16_t len);
extern uint8_t checksum_crc8_rohc(void *data, uint16_t len);
extern uint8_t checksum_crc8_itu(void *data, uint16_t len);
extern uint8_t checksum_crc8_maxim(void *data, uint16_t len);
extern uint8_t checksum_crc8_moorgen(void *data, uint16_t len);
/* crc32 */
extern uint32_t checksum_crc32_moorgen(void *data, uint16_t len);
/* other */
extern uint8_t checksum_xor(void *data, uint16_t len);
extern uint8_t checksum_revert_sum8(void *data, uint16_t len);
extern uint16_t checksum_sum16(void *data, uint16_t len);

#ifdef __cplusplus
}
#endif
#endif /* __CHECKSUM_H */
