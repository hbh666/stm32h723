#include "delayticks.h"

/*延时函数，不会将任务挂起*/
void delay_ticks(uint32_t delay)
{
    /*开始计数器值*/
    uint32_t tickstart = xTaskGetTickCount();
    /*指定延时时间*/
    uint32_t wait = delay;
    /*计算当前计数器值和开始计数器值的差*/
    uint32_t ticks_diff = xTaskGetTickCount() - tickstart;

    if ( ticks_diff < wait )
    {
        /*计算剩余等待的时间*/
        uint32_t remaining_ticks = wait - ticks_diff;

        /*循环等待，直到剩余时间结束*/
        while ( ( xTaskGetTickCount() - tickstart) < wait )
        {
            /*如果发生溢出，则重新计算时间差*/
            if ( xTaskGetTickCount() < tickstart )
            {
                /*重新计算时间差*/
                ticks_diff = UINT32_MAX - tickstart + xTaskGetTickCount();

                if ( ticks_diff < wait )
                {
                    remaining_ticks = wait - ticks_diff;
                }
                else
                {
                    /*如果时间差已经大于等于等待时间，则退出*/
                    break;
                }
            }
        }
    }
}