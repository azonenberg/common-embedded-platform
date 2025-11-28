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

#include <core/platform.h>
#include <stm32.h>
#include "MulticoreLogDevice.h"

#ifdef MULTICORE

MulticoreLogDevice::MulticoreLogDevice()
{
	for(uint32_t i=0; i<NUM_SECONDARY_CORES; i++)
	{
		m_channels[i] = nullptr;
		m_writePointers[i] = 0;
		memset(m_txBuffers[i], 0, LOG_TXBUF_SIZE);
	}
}

void MulticoreLogDevice::PrintBinary(char ch)
{
	auto nchan = GetCurrentCore();
	auto& wptr = m_writePointers[nchan];
	m_txBuffers[nchan][wptr] = ch;
	wptr ++;

	//Flush log buffer at end of line
	if( (wptr == LOG_TXBUF_SIZE) || (ch == '\n') )
		Flush();
}

char MulticoreLogDevice::BlockingRead()
{
	return 0;
}

void MulticoreLogDevice::Flush()
{
	//only flush the current core's log buffer to avoid potential races
	auto nchan = GetCurrentCore();
	auto& wptr = m_writePointers[nchan];
	auto pchan = m_channels[nchan];

	//Push to the IPC buffer
	if(pchan)
		pchan->GetSecondaryFifo().Push(reinterpret_cast<const uint8_t*>(m_txBuffers[nchan]), wptr);

	//and mark our local fifo as free
	wptr = 0;
}

#endif
