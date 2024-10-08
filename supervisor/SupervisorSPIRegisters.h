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

/**
	@file
	@author	Andrew D. Zonenberg
	@brief	SPI register definitions for supervisor
 */
#ifndef SupervisorSPIRegisters_h
#define SupervisorSPIRegisters_h

enum superregs_t
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// System identification

	//Present in all versions
	SUPER_REG_VERSION		= 0x00,		//Our version string
	SUPER_REG_IBCVERSION	= 0x01,		//IBC version string

	//Not present in legacy IBC
	SUPER_REG_IBCHWVERSION	= 0x02,		//IBC hardware version string

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// IBC sensors

	SUPER_REG_IBCVIN		= 0x10,		//IBC input voltage (nominal 48)
	SUPER_REG_IBCIIN		= 0x11,		//IBC input current
	SUPER_REG_IBCTEMP		= 0x12,		//IBC temperature measured near the converter
	SUPER_REG_IBCVOUT		= 0x13,		//IBC output voltage (nominal 12)
	SUPER_REG_IBCIOUT		= 0x14,		//IBC output current
	SUPER_REG_IBCVSENSE		= 0x15,		//IBC sense voltage (nominal 12)

	//Not present in legacy IBC
	SUPER_REG_IBCMCUTEMP	= 0x16,		//IBC temperature measured at the MCU
	SUPER_REG_IBC3V3		= 0x17,		//3V3_SB rail measured at the IBC MCU

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Supervisor MCU sensors

	SUPER_REG_MCUTEMP		= 0x18,		//Logic board temperature measured at the supervisor
	SUPER_REG_3V3			= 0x19		//3V3_SB rail measured at the supervisor

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Register IDs 0x1a to 3f available

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Registers 0x40 - 0x7f reserved for future OTA firmware update capability

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// Registers 0x80 - 0xff reserved for application specific functionality (will not be used by common code)
};

#endif
