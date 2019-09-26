# mijia BLE Low level API
## master分支
master分支为工作分支，由米家开发人员维护，抽象mijia BLE 底层通用 API 。mible_api.c 中函数为弱定义实现。api的具体说明详见https://miecosystem.github.io/mijia_ble_api/

master分支维护最新提交，每次发版本，会merge到release分支上。

## release分支
release分支为维护正式发布的版本，每个版本会有单独的tag标识，各芯片原厂分支必须从release分支的tag上checkout。 
release1.0 分支主要面向端设备应用。  release2.x分支补充完善gattc部分，主要面向网关设备应用。

## develop-2.x分支
增加网关api后的工作分支，由米家开发人员维护。每次发版本，会merge到release分支上。

## 芯片厂分支
各芯片平台分支从release分支checkout得到。 

各芯片厂提供兼容层适配：

保持原有release文件不变，增加各自平台上的适配文件，以芯片型号命名 xxxx_api.c，
如 nRF5_api.c 是 Nordic nRF5 平台对 mible_api.c 内函数的实现。

此文件中还可以增加其他文件，用于支持适配。

对于release 1.0 版本，芯片厂分支内的主分支文件，除 mible_port.h 外不能更改，mible_port.h 用于定义与平台相关的 printf、hexdump、malloc 等。
至少支持 256 bytes HEAP。

每个平台的适配完成某一release分支版本后，可以单独打一个分支版本号，格式为分支名-版本号，如silabs-v1.0.1。

## API测试

### Non-volatile Memory测试

由于实际产品中需要多次写入、读取数据，为保证产品稳定性，需验证SOC中NVM的可靠性。测试过程中需直接调用已经封装好的三个NVM相关API，通过循环读-改-写测试NVM的可靠性和极限。

米家要求NVM至少保证可反复擦写10000次，同时测量最大可支持擦写次数。

