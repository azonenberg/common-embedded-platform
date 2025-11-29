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

#ifndef IPCDescriptorTable_h
#define IPCDescriptorTable_h

#ifdef NUM_SECONDARY_CORES
#ifdef HAVE_IPCC

#include <peripheral/IPCC.h>

/**
	@brief Buffers and pointers for a unidirectional FIFO

	NO interlocking is performed in this class; that is the responsibility of IPCDescriptorChannel

	This is *not* a circular buffer: we push data until it's full or we hit a stopping point, then we stop
	and read the entire buffer into another location
 */
class UnidirectionalIPCFifo
{
public:

	UnidirectionalIPCFifo();

	uint32_t size()
	{ return m_size; }

	void Initialize(volatile uint8_t* buf, uint32_t size, uint32_t channel, volatile IPCC* ipcc, bool primaryTx)
	{
		m_buffer.Set(buf);
		m_ipcc.Set(ipcc);
		m_size = size;
		m_setmask = 1 << (16 + channel);
		m_clearmask = 1 << channel;
		m_primaryTx = primaryTx;
	}

	volatile uint8_t* GetBuffer()
	{ return m_buffer.Get(); }

	uint32_t ReadSize()
	{ return m_writePtr; }

	uint32_t WriteSize()
	{ return m_size - m_writePtr; }

	void Push(const uint8_t* buf, uint32_t size);
	uint32_t Pop(uint8_t* rxbuf);

	bool Peek();

protected:

	///@brief The actual data buffer
	PaddedPointer<uint8_t> m_buffer;

	///@brief The IPC controller
	PaddedPointer<IPCC> m_ipcc;

	///@brief Size of the buffer
	volatile uint32_t m_size;

	///@brief Write pointer
	volatile uint32_t m_writePtr;

	///@brief Channel ID set mask
	uint32_t m_setmask;

	///@brief Channel ID clear mask
	uint32_t m_clearmask;

	///@brief True if primary -> secondary path
	bool m_primaryTx;
};

/**
	@brief A single channel within the IPC descriptor table
 */
class IPCDescriptorChannel
{
public:
	IPCDescriptorChannel();

	UnidirectionalIPCFifo& GetPrimaryFifo()
	{ return m_primaryTxFifo; }

	UnidirectionalIPCFifo& GetSecondaryFifo()
	{ return m_secondaryTxFifo; }

	void SetName(const char* ptr)
	{ m_name.Set(ptr); }

	const char* GetName()
	{ return const_cast<const char*>(m_name.Get()); }

	void Print(unsigned int idx);

protected:

	///@brief Name of the channel
	PaddedPointer<const char> m_name __attribute__((aligned(8)));

	///@brief FIFO from primary to secondary
	UnidirectionalIPCFifo m_primaryTxFifo __attribute__((aligned(8)));

	///@brief FIFO from secondary to primary
	UnidirectionalIPCFifo m_secondaryTxFifo __attribute__((aligned(8)));
};

/**
	@brief Descriptor table for interprocess communication

	Must be fully populated by the primary core before secondary cores are started and not modified after that.

	Must use stdint types only, and be structured to have the same memory layout on armv8-m and aarch64.
 */
class IPCDescriptorTable
{
public:
	IPCDescriptorTable(volatile ipcc_t* ipcc);

	void Print();

	#ifdef PRIMARY_CORE
	IPCDescriptorChannel* AllocateChannel(
		const char* name,
		volatile uint8_t* txbuf, uint32_t txsize,
		volatile uint8_t* rxbuf, uint32_t rxsize
		);
	#else
	IPCDescriptorChannel* FindChannel(const char* name);
	#endif

	IPCDescriptorChannel* GetChannelByIndex(uint32_t i)
	{ return &m_channels[i]; }

protected:

	///@brief The actual IPC channel data descriptors
	IPCDescriptorChannel m_channels[NUM_IPC_CHANNELS];

	///@brief The IPCC channel we're using
	IPCC m_ipcc;

	///@brief Index of the first free channel
	uint32_t m_firstFreeChannel __attribute__((aligned(8)));
};

extern "C" IPCDescriptorTable g_ipcDescriptorTable;

#endif
#endif
#endif
