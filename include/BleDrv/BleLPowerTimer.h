/**
 * @file				BleLPowerTimer.h
 * @brief			BleLPowerTimer.c 头文件
 * @author			fengxun
 * @date			2017年11月3日

*/
#ifndef __BLELPOWERTIMER_H__
#define __BLELPOWERTIMER_H__
#include "../DhGlobalHead.h"


/*
* @brief 	利用RTC实现3个ble需要使用的低功耗时钟,定时器每次只作用一次。
			LP_TIMER0: 用来定时长时间的定时器(几十ms-几十s)，使用前会重启底层硬件RTC，使用RTC0的 compare0 寄存器实现
			LP_TIMER1: 用来定时短时间的定时器(几百us-几ms)，在长定时器启动的情况下才能使用，使用RTC0的 compare1 寄存器实现
			LP_TIMER2: 同LP_TIMER1，使用RTC0的 compare2 寄存器实现
*/

typedef enum
{
	BLE_LP_TIMER0 = 0,
	BLE_LP_TIMER1,
	BLE_LP_TIMER2,
	BLE_LP_MAX,
	BLE_LP_TIMER_INVALID,
}EnBleLPowerTimer;

typedef enum
{
	LP_CLK_SRC_RC_250_PPM = 0,		// 目前只实现使用内部RC的方式
}EnBleLPowerClkSrc;
typedef void (*BleLPowerTimeroutHandler)(void * pvalue);

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

extern void BleLPowerTimerInit( EnBleLPowerClkSrc src, u4 u4CalTimeoutMs );
extern u4 BleLPowerTimerStart( EnBleLPowerTimer timerId, u4 timeoutUs, BleLPowerTimeroutHandler cb, void *pvalue);
extern u4 BleLPowerTimerStop( EnBleLPowerTimer timerId);
extern u4 BleLPowerTimerTickGet(void);



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __BLELPOWERTIMER_H__ */
