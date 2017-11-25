/*****************************************************************************
File Name:    easy_common.h
Discription:  
History:
Date                Author                   Description
2017-11-11         Lucien                    Creat
****************************************************************************/
#ifndef __EASY_COMMON_H__
#define __EASY_COMMON_H__



#include "ll.h"
#define DISABLE_IRQ




#ifdef DISABLE_IRQ
#define disable_irqs() GLOBAL_INT_DISABLE()
#define enable_irqs() GLOBAL_INT_RESTORE()
#else
#define disable_irqs() {}
#define enable_irqs() {}
#endif









#endif






