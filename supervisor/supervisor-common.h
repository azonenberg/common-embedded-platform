/***********************************************************************************************************************
*                                                                                                                      *
* common-embedded-platform                                                                                             *
*                                                                                                                      *
* Copyright (c) 2023-2024 Andrew D. Zonenberg and contributors                                                         *
* All rights reserved.                                                                                                 *
*                                                                                                                      *
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the     *
* following conditions are met:                                                                                        *
*                                                                                                                      *
*    * Redistributions of source code must retain the above copyright notice, this list of conditions, and the         *
*      following disclaimer.                                                                                           *
*                                                                                                                      *
*    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the       *
*      following disclaimer in the documentation and/or other materials provided with the distribution.                *
*                                                                                                                      *
*    * Neither the name of the author nor the names of any contributors may be used to endorse or promote products     *
*      derived from this software without specific prior written permission.                                           *
*                                                                                                                      *
* THIS SOFTWARE IS PROVIDED BY THE AUTHORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED   *
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL *
* THE AUTHORS BE HELD LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES        *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR       *
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE       *
* POSSIBILITY OF SUCH DAMAGE.                                                                                          *
*                                                                                                                      *
***********************************************************************************************************************/

#ifndef supervisor_common_h
#define supervisor_common_h

#include <core/platform.h>

#include <peripheral/ADC.h>
#include <peripheral/GPIO.h>
#include <peripheral/I2C.h>
#include <peripheral/SPI.h>
#include <peripheral/UART.h>

#include <embedded-utils/FIFO.h>
#include <embedded-utils/StringBuffer.h>

#include <bootloader/bootloader-common.h>
#include <bootloader/BootloaderAPI.h>

//TODO: fix this path somehow?
#include "../../../common-ibc/firmware/main/regids.h"

extern char g_version[20];
extern char g_ibcSwVersion[20];
extern char g_ibcHwVersion[20];

extern const uint8_t g_tempI2cAddress;
extern const uint8_t g_ibcI2cAddress;

extern I2C g_i2c;

void Super_Init();
void Super_InitI2C();
void Super_InitIBC();

#ifdef HAVE_ADC
extern ADC* g_adc;
void Super_InitADC();
#endif

//Global hardware config used by both app and bootloader
extern UART<16, 256> g_uart;
extern SPI<64, 64> g_spi;
extern GPIOPin* g_spiCS;

extern volatile BootloaderBBRAM* g_bbram;

extern uint16_t g_ibcTemp;
extern uint16_t g_ibc3v3;
extern uint16_t g_ibcMcuTemp;
extern uint16_t g_vin48;
extern uint16_t g_vout12;
extern uint16_t g_voutsense;
extern uint16_t g_iin;
extern uint16_t g_iout;
extern uint16_t g_3v3Voltage;
extern uint16_t g_mcutemp;

bool PollIBCSensors();

#endif
