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

#ifndef BootloaderAPI_h
#define BootloaderAPI_h

/**
	@brief Command/status API exposed by the bootloader to the application, through backup SRAM
 */

//Boot state machine
enum BootloaderState
{
	STATE_POR		= 0x00,			//we just booted for the first time since powerup
	STATE_APP		= 0x01,			//Application was launched, no fault detected
	STATE_DFU		= 0x02,			//Application requested we enter DFU mode
	STATE_CRASH		= 0x03			//Application crash handler called
};

enum CrashReason
{
	CRASH_UNUSED_ISR	= 0x00,
	CRASH_NMI			= 0x01,
	CRASH_HARD_FAULT	= 0x02,
	CRASH_BUS_FAULT		= 0x03,
	CRASH_USAGE_FAULT	= 0x04,
	CRASH_MMU_FAULT		= 0x05
};

//BBRAM content
struct __attribute__((packed)) BootloaderBBRAM
{
	BootloaderState m_state;
	CrashReason		m_crashReason;
};

extern volatile BootloaderBBRAM* g_bbram;

///@brief size of a .gnu.build-id block including headers
#define GNU_BUILD_ID_SIZE (uint32_t)36

///@brief SIze of a .gnu.build-id block as hex (including null terminator)(
#define GNU_BUILD_ID_HEX_SIZE 41

#endif
