# Supervisor

This code is a skeleton for a (typically) STM32L431 based power/reset supervisor that manages multiple rails

Common features:
* Interfaces to [common-ibc](https://github.com/azonenberg/common-ibc) over I2C to poll sensors
* Monitors internal temperature and supply voltage
