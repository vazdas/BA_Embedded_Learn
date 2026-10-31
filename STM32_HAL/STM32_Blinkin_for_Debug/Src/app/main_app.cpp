#include "main.h"
#include <stdint.h>

void delay_ms(uint32_t ms) {
    uint32_t count = ms * 20000;
    for (volatile uint32_t i = 0; i < count; i++) {
    }
}

extern "C" void main_cpp() {
    uint32_t blink_period= 500;

    while (1)
    {
        GPIOC->ODR ^= (1 << 13);

        if ((GPIOA->IDR & (1 << 0)) == 0) {
            blink_period = 100 + (SysTick->VAL % 900);
            delay_ms(200);
        }

        delay_ms(blink_period);
    }

}
