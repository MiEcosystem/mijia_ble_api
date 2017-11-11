# Mijia BLE Low level API

如何进行兼容层适配：
1. 在 mible_port.h 里定义平台相关的 printf 和 hexdump 函数。
2. 利用 SoC SDK 实现 mible_api.c 中函数接口。

e.g.
branch nordic 提供 nordic demo