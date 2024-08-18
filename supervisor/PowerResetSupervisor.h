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

#ifndef PowerResetSupervisor_h
#define PowerResetSupervisor_h

#include "RailDescriptor.h"
#include "ResetDescriptor.h"
#include <etl/vector.h>

/**
	@brief Top level control class for supervisor logic
 */
class PowerResetSupervisor
{
public:
	PowerResetSupervisor(etl::ivector<RailDescriptor*>& rails, etl::ivector<ResetDescriptor*>& resets)
		: m_railSequence(rails)
		, m_resetSequence(resets)
		, m_powerOn(false)
		, m_resetsDone(false)
		, m_resetSequenceIndex(0)
	{}

	bool IsPowerOn()
	{ return m_powerOn; }

	bool IsResetsDone()
	{ return m_resetsDone; }

	///@brief Shuts down all rails in reverse order but without any added sequencing delays
	void PanicShutdown();

	void PowerOn();
	void PowerOff();

	/**
		@brief Called each iteration through the main loop
	 */
	void Iteration()
	{
		if(m_powerOn)
		{
			UpdateResets();
			MonitorRails();
		}
	}

protected:

	void UpdateResets();
	void MonitorRails();

	///@brief Hook called at the end of the power-on sequence
	virtual void OnPowerOn()
	{}

	///@brief Hook called at the end of the power-off sequence
	virtual void OnPowerOff()
	{}

	///@brief Called by PanicShutdown() when there's a fatal failure
	virtual void OnFault()
	{
		while(1)
		{}
	};

	///@brief The rail sequence
	etl::ivector<RailDescriptor*>& m_railSequence;

	///@brief The reset sequence
	etl::ivector<ResetDescriptor*>& m_resetSequence;

	///@brief True if power is all the way on
	bool m_powerOn;

	///@brief True if all resets are currently up
	bool m_resetsDone;

	///@brief Index of the currently active line in the reset state machine
	size_t m_resetSequenceIndex;
};

#endif
