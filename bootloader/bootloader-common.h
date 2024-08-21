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

#ifndef bootloader_common_h
#define bootloader_common_h

#include "BootloaderAPI.h"

#include <peripheral/CRC.h>
#include <peripheral/Flash.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Common globals with pointers to various regions of flash

///@brief Pointer to the application region of flash
extern uint32_t* const	g_appVector;

///@brief Size of application region of flash
extern const uint32_t	g_appImageSize;

///@brief Offset of the application version string within flash (interrupt vector table size plus 32-byte alignment)
extern const uint32_t	g_appVersionOffset;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// KVS keys for bootloader state

extern const char* g_imageVersionKey;
extern const char* g_imageCRCKey;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hooks for customizing the bootloader

/**
	@brief Board-specific bootloader initialization

	As a minimum, this function should define two storage banks and call InitKVS() on them
 */
void Bootloader_Init();

/**
	@brief Clears any command buffer data that showed up while we were busy, to prevent overflows
 */
void Bootloader_ClearRxBuffer();

/**
	@brief Do any final processing before the application launches

	As a minimum, the UART transmit FIFO should be flushed to ensure that all debug log messages from the bootloader
	are printed before the application takes control
 */
void Bootloader_FinalCleanup();

/**
	@brief Run the "firmware update" mode of the bootloader

	This function should provide some sort of command interface (via SPI, serial, Ethernet, etc) for pushing a new
	firmware image to the device.

	It is called if the user requests a firmware update, or if no bootable image was found in flash
 */
void __attribute__((noreturn)) Bootloader_FirmwareUpdateFlow();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootloader methods called by the main loop or DFU flow

/**
	@brief Main event loop for the bootloader
 */
void Bootloader_MainLoop();

/**
	@brief Check if the provided app partition contains what looks like a valid image

	@param appVector	Pointer to application partition
 */
bool ValidateAppPartition(const uint32_t* appVector);

/**
	@brief Checks if the application partition contains a different firmware version than we last booted

	@param appVector	Pointer to application partition

	@return True if app version has changed
 */
bool IsAppUpdated(const uint32_t* appVector);

/**
	@brief Gets the application version hash

	Returns false on failure
 */
bool GetImageVersion(const uint32_t* appVector, char* versionString);

/**
	@brief Jump to the application partition and launch it
 */
void __attribute__((noreturn)) BootApplication(const uint32_t* appVector);

/**
	@brief Assembly helper called by BootApplication
 */
extern "C" void __attribute__((noreturn)) DoBootApplication(const uint32_t* appVector);

/**
	@brief Erases the application flash partition
 */
void EraseFlash(uint32_t* appVector);

#endif
