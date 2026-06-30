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
#include "IPCDescriptorTable.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The descriptor table

//for now we have only one table going from CPU1 to CPU2
__attribute__((section(".ipcdescriptors"))) IPCDescriptorTable g_ipcDescriptorTable(&IPCC1);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPCDescriptorTable

IPCDescriptorTable::IPCDescriptorTable(volatile ipcc_t* ipcc)
	: m_ipcc(ipcc)	//initialize on both sides
{
	#ifdef PRIMARY_CORE
		m_firstFreeChannel = 0;

		m_ipcc.Initialize();
	#endif
}

#ifdef PRIMARY_CORE

/**
	@brief Allocate a new IPC channel with a given name and buffer

	The name and buffers are stored in the channel without copying and must remain available for the
	lifetime of the object.
 */
IPCDescriptorChannel* IPCDescriptorTable::AllocateChannel(
	const char* name,
	volatile uint8_t* txbuf, uint32_t txsize,
	volatile uint8_t* rxbuf, uint32_t rxsize
	)
{
	//Make sure we have channels to allocate
	if(m_firstFreeChannel >= NUM_IPC_CHANNELS)
		return nullptr;

	//Get the newly allocated block
	auto idx = m_firstFreeChannel;
	auto chan = &m_channels[idx];
	m_firstFreeChannel = idx + 1;	//gcc complains if we ++ a volatile
									//(even though only primary side can write to it)

	//Initialize it
	chan->SetName(name),
	chan->GetPrimaryFifo().Initialize(txbuf, txsize, idx, &m_ipcc, true);
	chan->GetSecondaryFifo().Initialize(rxbuf, rxsize, idx, &m_ipcc, false);

	//Done
	return chan;
}
#else

IPCDescriptorChannel* IPCDescriptorTable::FindChannel(const char* name)
{
	for(uint32_t i=0; i<NUM_IPC_CHANNELS; i++)
	{
		auto chan = &m_channels[i];

		//Unallocated? Skip it
		auto cname = chan->GetName();
		if(cname == nullptr)
			continue;

		//Check name
		if(!strcmp(cname, name))
			return chan;
	}

	return nullptr;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

UnidirectionalIPCFifo::UnidirectionalIPCFifo()
{
	#ifdef PRIMARY_CORE
		//Clear to empty
		m_buffer.Set(nullptr);
		m_size = 0;
		m_writePtr = 0;
		m_setmask = 0;
		m_clearmask = 0;
	#endif
}

void UnidirectionalIPCFifo::Push(const uint8_t* buf, uint32_t size)
{
	//early out if buffer is too big
	if(size > m_size)
		return;

	//Wait until the IPC channel is free
	if(m_primaryTx)
	{
		while(!m_ipcc->IsPrimaryToSecondaryChannelFree(m_clearmask))
		{}
	}
	else
	{
		while(!m_ipcc->IsSecondaryToPrimaryChannelFree(m_clearmask))
		{}
	}

	//Write it
	auto wbuf = const_cast<uint8_t*>(m_buffer.Get());
	memcpy(wbuf, buf, size);
	m_writePtr = size;

	//Cache flush
	CleanDataCache(wbuf, size);
	CleanDataCache((void*)&m_writePtr, sizeof(m_writePtr));

	//Mark it as busy
	if(m_primaryTx)
		m_ipcc->SetPrimaryToSecondaryChannelBusy(m_setmask);
	else
		m_ipcc->SetSecondaryToPrimaryChannelBusy(m_setmask);

	//TODO: cache flush
	asm("dmb st");
}

bool UnidirectionalIPCFifo::Peek()
{
	if(m_primaryTx)
		return !m_ipcc->IsPrimaryToSecondaryChannelFree(m_clearmask);
	else
		return !m_ipcc->IsSecondaryToPrimaryChannelFree(m_clearmask);
}

/**
	@brief Pops the rx buffer into a user-supplied buffer, which must be at least m_size bytes long.

	@return the actual number of bytes read
 */
uint32_t UnidirectionalIPCFifo::Pop(uint8_t* rxbuf)
{
	//No data ready? Abort
	if(!Peek())
		return 0;

	uint32_t p = m_writePtr;

	memcpy(rxbuf, const_cast<uint8_t*>(m_buffer.Get()), p);
	asm("dmb st");

	if(m_primaryTx)
		m_ipcc->SetPrimaryToSecondaryChannelFree(m_clearmask);
	else
		m_ipcc->SetSecondaryToPrimaryChannelFree(m_clearmask);

	return p;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPCDescriptorChannel

IPCDescriptorChannel::IPCDescriptorChannel()
{
	#ifdef PRIMARY_CORE
		m_name.Set(nullptr);
	#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Debug logging

void IPCDescriptorChannel::Print(unsigned int idx)
{
	const char* name = GetName();
	g_log("%2u | %-15s | %08x | %8u | %08x | %8u\n",
		idx,
		name ? name : "(null)",
		m_primaryTxFifo.GetBuffer(),
		m_primaryTxFifo.size(),
		m_secondaryTxFifo.GetBuffer(),
		m_secondaryTxFifo.size()
		);}

void IPCDescriptorTable::Print()
{
	g_log("Dumping IPC descriptor table (%d secondary cores, %d channels)\n", NUM_SECONDARY_CORES, NUM_IPC_CHANNELS);
	LogIndenter li(g_log);

	g_log("ch | %-15s | %-8s | %8s | %-8s | %8s\n", "Name", "TX buf", "TX size", "RX buf", "RX size");
	g_log("-----------------------------------------------------------------------------------------------\n");

	for(size_t i=0; i<NUM_IPC_CHANNELS; i++)
		m_channels[i].Print(i);
}
