# Mijia BLE Low level API

master分支为Mijia BLE底层通用API定义。mible_api.c中函数为弱定义实现。

在各芯片厂分支中提供兼容层适配：
1. 在 XXXX_port.h 里定义平台相关的 printf函数 、hexdump 函数、 malloc等。
2. 根据各平台协议栈自身情况实现 XXXX_api.c 函数，覆盖mible_api.c接口。

e.g.
branch nordic 提供 nordic demo
