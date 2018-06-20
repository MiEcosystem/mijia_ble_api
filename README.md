# Mijia BLE Low level API
## 主分支
Mijia BLE底层通用API定义。mible_api.c中函数为弱定义实现。
## 芯片厂分支
各芯片厂提供兼容层适配：
保持原有主分支文件不变，增加各自平台上的适配文件，以芯片型号命名（xxxx_api.c），如nRF5_api.c是nordic nRF5平台对mible_api.c内函数的实现。此文件中还可以增加其他文件，用于支持适配。芯片厂分支内的主分支文件，除mible_port.h外不能更改，mible_port.h可以更改，用于定义与平台相关的printf函数、hexdump函数、malloc函数等。
