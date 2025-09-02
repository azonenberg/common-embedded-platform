# Standard boilerplate for STM32H750





----------------------------
FIXME THIS IS ALL OUT OF DATE
* Log timer using TIM2 at 10 kHz
* KVS using last two sectors of internal flash
* SMPS-LDO cascade with 1.8V SMPS
* External 25 MHz HSE oscillator
* PLL1 supplies CPU (500 MHz), kernel clock Q (50 MHz), SWO clock (50 MHz / 25 Mbps)
* PLL2 R supplies FMC (200 MHz kernel = 100 MHz FMC clock)
* AHB at 250 MHz
* All APB at 62.5 MHz
* PLL1 Q (50 MHz) selected as RNG clock
* 128 kB MicroKVS using last two sectors of internal flash
* RTC using HSE clock
