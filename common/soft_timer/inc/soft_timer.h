/**
 * @file common/utils/soft_timer/inc/soft_timer.h
 *
 * Copyright (C) 2022
 *
 * soft_timer.h is free software: you can redistribute it and/or modify
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
#ifndef __SOFT_TIMER_H
#define __SOFT_TIMER_H

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
    SFTIM_MODE_SINGLE = 0,
    SFTIM_MODE_REPEAT = !SFTIM_MODE_SINGLE
} soft_timer_mode_t;

typedef struct timer_tcb *timer_handle_t;

typedef void (*timer_cb_t)(timer_handle_t);

/*---------- variable prototype ----------*/
/*---------- function prototype ----------*/
/**
 * @brief Create a new software timer instance, and return a handle by which the
 * created software timer can be referenced.
 * Timers are created in the dormant state. The soft_timer_start(), soft_timer_restart(),
 * soft_timer_change_period() API functions can all be used to transition a timer into
 * the active state.
 * @param name A text name that is assigned to the timer. This is done purely to assist debugging.
 * The kernel itself only ever references a timer by its handle, and never by its name.
 * @param mode If mode is set to SFTIM_MODE_REPEAT the the timer will expire repeatedly with a
 * frequency set by the period parameter.
 * If mode is set to SFTIM_MODE_SINGLE then the timer will be a noe-shot timer adn enter the
 * dormant state after it expires.
 * @param period The timer period. If the timer period is set to 100, it means that soft_timer_tick()
 * will be call 100 counts and then cb will be called.
 * @param user_data User private data pointer.
 * @param cb The function to call when the timer expires.
 * CallBack fundtions must have the prototype defined by timer_cb_t, which is "void callback(timer_handle_t timer)"
 * 
 * @retval If the timer is successfully created then a handle to the newly created timer is returned.
 * If the timer cannot be created because there is insufficient heap remaining to allocate the timer
 * structers then NULL is returned.
 */
extern timer_handle_t soft_timer_create(const char *name, soft_timer_mode_t mode, uint32_t period, void *user_data, timer_cb_t cb);

/**
 * @brief Delete a timer that was previously created using the soft_timer_create() API function
 * @param timer The handle of the timer being deleted.
 * 
 * @retval None
 */
extern void soft_timer_destroy(timer_handle_t timer);

/**
 * @brief Starts a timer that was previously created using the soft_timer_create() API function.
 * If the timer had already been started and was already in the active state, the soft_timer_start()
 * has equivalent functionality to the soft_timer_restart() API function.
 * @param timer The handle of the timer being started.
 * 
 * @retval None
 */
extern void soft_timer_start(timer_handle_t timer);

/**
 * @brief Re-starts a timer that was previously created using the soft_timer_create() API function.
 * If the timer had already been started and was already in the active state, then soft_timer_restart()
 * will cause the timer to re-evaluate its expiry time so that it is relative to when soft_timer_restart()
 * was called. If the timer was in the dormant state then soft_timer_retsart() has equivalent functionality
 * to the soft_timer_start() API function.
 * @param timer The handle of the timer being restarted.
 * 
 * @retval None
 */
extern void soft_timer_restart(timer_handle_t timer);

/**
 * @brief Stops a timer that was previously started using either of soft_timer_start(), soft_timer_restart()
 * or soft_timer_change_period() API functions
 * Stopping a timer ensures the timer is not in the active state.
 * @param timer The handle if the timer being stopped.
 * 
 * @retval None
 */
extern void soft_timer_stop(timer_handle_t timer);

/**
 * @brief Changes the period of a timer that was previously created using the soft_timer_create() API function.
 * soft_timer_change_period() can be called to change the period of an active or dormant state timer.
 * @param timer The handle of the timer that is having its period chaned.
 * @param period The new period for timer. Timer periods are specified in tick periods.
 * 
 * @retval None
 */
extern void soft_timer_change_period(timer_handle_t timer, uint32_t period);

/**
 * @brief Updates a timer to be either an auto-reload timer, in which case the timer automatically resets
 * itself each time it expires, or a one-shot timer, in which case the timer will only expire once unless
 * it is manually restarted.
 * @param timer The handle of the timer being updated.
 * @param mode If mode is set to SFTIM_MODE_REPEAT then the timer will expire repeatedly with a frequency
 * set by the timer's period.
 * If mode is set to SFTIM_MODE_SINGLE then the timer will be a one-shot timer and enter the dormant state
 * after it expires.
 */
extern void soft_timer_set_reload_mode(timer_handle_t timer, soft_timer_mode_t mode);

/**
 * @brief Queries a timer to see if it is active or dormant.
 * A timer will be dormant if:
 *  1) It has been created but not startr, or
 *  2) It is an expired one-shot timer that has not been restarted.
 * @param timer The timer being queried.
 * 
 * @retval true will be returned if the timer is active. false will be returned if the timer is dormant.
 */
extern bool soft_timer_is_active(timer_handle_t timer);

/**
 * @brief Returns the name assigned to the timer.
 * @param timer The timer being queried.
 * 
 * @retval The name assigned to the timer being queried.
 */
extern const char *soft_timer_get_name(timer_handle_t timer);

/**
 * @brief Queries a timer to determine if it is an auto-reload timer, in which case the timer
 * automatically resets itself each time it expires, or a one-shot timer, in which case the timer
 * will only expire once unless it is manually retsarted.
 * @param timer The handle of the timer being queried.
 * 
 * @retval If the timer is an auto-reload timer then SFTIM_MODE_REPEAT is returned.
 * If the timer is a one-shot timer then SFTIM_MODE_SINGLE is returned.
 */
extern soft_timer_mode_t soft_timer_get_reload_mode(timer_handle_t timer);

/**
 * @brief Queries period of a timer.
 * @param timer The handle of the timer being queried.
 * 
 * @retval The period of the timer in ticks.
 */
extern uint32_t soft_timer_get_period(timer_handle_t timer);

/**
 * @brief Queries user data of a timer.
 * @param timer The handle of the timer being queried.
 * 
 * @retval The user data of the timer.
 */
extern void *soft_timer_get_user_data(timer_handle_t timer);

/**
 * @brief Check a timer state if it is ready to do callback function.
 * If a timer is an auto-reload timer, starts the timer again after doing callback function.
 * If a timer is a one-shot timer, enter the dormant state after doing callback function.
 * 
 * @retval None
 */
extern void soft_timer_poll(void);

/**
 * @brief Timer tick handle, it should be called on every tick.
 * 
 * @retval None
 */
extern void soft_timer_tick(void);

#ifdef __cplusplus
}
#endif
#endif /* __SOFT_TIMER_H */
