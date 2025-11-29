/***********************************************************************************************************************
*                                                                                                                      *
* common-embedded-platform                                                                                             *
*                                                                                                                      *
* Copyright (c) 2024 Andrew D. Zonenberg and contributors                                                              *
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

#ifndef platform_h
#define platform_h

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stm32.h>

#include <etl/vector.h>

#include <peripheral/RCC.h>
#include <peripheral/Timer.h>

#include <embedded-utils/Logger.h>
#include <microkvs/kvs/KVS.h>

#include "../../embedded-utils/LogSink.h"

//Common globals every system expects to have available
extern Logger g_log;
extern Timer g_logTimer;
extern KVS* g_kvs;

//Global helper functions
void __attribute__((noreturn)) Reset();
void InitKVS(StorageBank* left, StorageBank* right, uint32_t logsize);
void FormatBuildID(const uint8_t* buildID, char* strOut);
void PrintCortexMInfo();

#ifdef __aarch64__
void PrintCortexAInfo();
#endif

//Returns true in bootloader, false in application firmware
bool IsBootloader();

//Task types
#include "Task.h"
#include "TimerTask.h"

#include "bsp.h"

//MULTI CORE flow
#ifdef MULTICORE

	//TODO

//SINGLE CORE flow
#else
	//All tasks
	extern etl::vector<Task*, MAX_TASKS>  g_tasks;

	//Timer tasks (strict subset of total tasks)
	extern etl::vector<TimerTask*, MAX_TIMER_TASKS>  g_timerTasks;
#endif

//Helpers for FPGA interfacing
void InitFPGA();

extern uint8_t g_fpgaSerial[12];
extern uint32_t g_usercode;

#ifndef MAX_LOG_SINKS
#define MAX_LOG_SINKS 2
#endif
extern LogSink<MAX_LOG_SINKS>* g_logSink;

//Callbacks for multicore init
#ifdef MULTICORE
extern "C" void hardware_init_hook();
extern "C" void CoreInit(unsigned int core);
extern "C" void CoreMain(unsigned int core);
#endif

#endif
