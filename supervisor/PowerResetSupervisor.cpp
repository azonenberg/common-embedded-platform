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

#include <core/platform.h>
#include <supervisor/PowerResetSupervisor.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fault handling paths

void PowerResetSupervisor::PanicShutdown()
{
	//Shut down all rails in reverse order
	for(int i = m_railSequence.size()-1; i >= 0; i--)
		m_railSequence[i]->TurnOff();

	//Assert all resets (don't care about order, we're powered down anyway)
	for(auto r : m_resetSequence)
		r->Assert();

	//Clear status flags to indicate we're not running
	m_powerOn = false;
	m_resetsDone = false;
	m_resetSequenceIndex = 0;

	g_log("Panic shutdown completed\n");

	//Let the base class do any other processing
	OnFault();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Normal on/off path

/**
	@brief Turn on power, blocking until it's fully up
 */
void PowerResetSupervisor::PowerOn()
{
	g_log("Turning power on\n");

	//Turn on all rails and wait for them all to come up
	for(auto rail : m_railSequence)
	{
		LogIndenter li(g_log);
		if(!rail->TurnOn())
		{
			PanicShutdown();
			return;
		}
	}

	//Start the reset sequence
	g_log("Releasing resets\n");
	m_powerOn = true;
	m_resetsDone = false;
	m_resetSequenceIndex = 0;
	if(!m_resetSequence.empty())
		m_resetSequence[0]->Deassert();

	OnPowerOn();
}

/**
	@brief Turn off power, blocking until it's fully down
 */
void PowerResetSupervisor::PowerOff()
{
	g_log("Turning power off\n");

	//Assert all resets in reverse order
	for(int i = m_resetSequence.size()-1; i >= 0; i--)
		m_resetSequence[i]->Assert();

	//Turn all rails off, waiting 1ms between them
	for(int i = m_railSequence.size()-1; i >= 0; i--)
	{
		m_railSequence[i]->TurnOff();
		g_logTimer.Sleep(10);
	}

	//Power is now off
	m_powerOn = false;
	m_resetsDone = false;
	m_resetSequenceIndex = 0;

	OnPowerOff();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Reset state machine

/**
	@brief Run the reset state machine
 */
void PowerResetSupervisor::UpdateResets()
{
	//Actively running reset sequence. Time to advance the sequence?
	if(!m_resetsDone && m_resetSequence[m_resetSequenceIndex]->IsReady())
	{
		m_resetSequenceIndex ++;

		//Done with all resets? We're OK
		if(m_resetSequenceIndex >= m_resetSequence.size())
		{
			g_log("Reset sequence complete\n");
			m_resetsDone = true;
		}

		//Nope, clear the next reset in line
		else
			m_resetSequence[m_resetSequenceIndex]->Deassert();
	}

	//Check all devices earlier in the reset sequence and see if any went down.
	//If so, back up to that stage and resume the sequence
	for(size_t i=0; i<m_resetSequenceIndex; i++)
	{
		if(!m_resetSequence[i]->IsReady())
		{
			g_log("%s is no longer ready, restarting reset sequence from that point\n",
				m_resetSequence[i]->GetName());
			m_resetSequenceIndex = i;
			m_resetsDone = false;

			//Assert all subsequent resets
			for(size_t j=i+1; j<m_resetSequence.size(); j++)
				m_resetSequence[j]->Assert();
			break;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Detect loss of power or rail failures and cleanly shut down

void PowerResetSupervisor::MonitorRails()
{
	for(auto rail : m_railSequence)
	{
		if(!rail->IsPowerGood())
		{
			g_log(Logger::ERROR, "Rail %s power failure - panic shutdown\n", rail->GetName());
			PanicShutdown();
			return;
		}
	}
}
