/***********************************************************************************************************************
*                                                                                                                      *
* common-embedded-platform                                                                                             *
*                                                                                                                      *
* Copyright (c) 2024-2025 Andrew D. Zonenberg and contributors                                                         *
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

#ifndef MulticoreLogDevice_h
#define MulticoreLogDevice_h

#ifdef MULTICORE

#ifndef LOG_TXBUF_SIZE
#define LOG_TXBUF_SIZE 256
#endif

#include "IPCDescriptorTable.h"

/**
	@brief Log device that logs to one of several IPC descriptor tables depending on the current core ID
 */
class MulticoreLogDevice : public CharacterDevice
{
public:
	MulticoreLogDevice();

	void LookupChannel(uint32_t i, const char* name)
	{
		if(i < NUM_SECONDARY_CORES)
			m_channels[i] = g_ipcDescriptorTable.FindChannel(name);
	}

	virtual void PrintBinary(char ch) override;
	virtual char BlockingRead() override;
	virtual void Flush() override;

protected:

	///@brief The IPC channels to the other core
	IPCDescriptorChannel* m_channels[NUM_SECONDARY_CORES];

	///@brief FIFOs for accumulating log data we haven't yet pushed to the other core
	char m_txBuffers[NUM_SECONDARY_CORES][LOG_TXBUF_SIZE];

	///@brief Write pointers
	uint32_t m_writePointers[NUM_SECONDARY_CORES];
};

#endif
#endif
