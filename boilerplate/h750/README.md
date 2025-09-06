# Standard boilerplate for STM32H750

* Log timer using TIM2 at 10 kHz
* KVS using last two sectors of external memory mapped QSPI flash
* LDO only (no SMPS in the H750) at VOS0
* External 25 MHz HSE oscillator
* PLL1 supplies CPU (475 MHz), kernel clock Q (47.5 MHz), SWO clock (95 MHz / 47.5 Mbps)
* PLL2 R supplies FMC (150 MHz kernel = 75 MHz FMC clock)
* AHB at 237.5 MHz
* All APB at 118.75 MHz
* PLL1 Q (47.5 MHz) selected as RNG clock
* RTC using HSE clock
