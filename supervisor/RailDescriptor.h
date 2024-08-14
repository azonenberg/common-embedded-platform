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

#ifndef RailDescriptor_h
#define RailDescriptor_h

/**
	@brief Base class for control signals for a single power rail
 */
class RailDescriptor
{
public:

	RailDescriptor(const char* name)
		: m_name(name)
	{}

	///@brief Turns on the rail, returns true on success
	virtual bool TurnOn() =0;

	///@brief Turns off the rail
	virtual void TurnOff() =0;

protected:
	const char* m_name;
};

/**
	@brief A power rail that has an active-high enable line, but no feedback on whether it's up
 */
class RailDescriptorWithEnable : public RailDescriptor
{
public:

	/**
		@brief Creates a new rail descriptor

		@param name		Human readable rail name for logging
		@param enable	Enable pin (set high to turn on power)
		@param timer	Timer to use for sequencing delays
		@param delay	Delay, in timer ticks, after turning on this rail before doing anything else
	 */
	RailDescriptorWithEnable(const char* name, GPIOPin& enable, Timer& timer, uint16_t delay)
		: RailDescriptor(name)
		, m_enable(enable)
		, m_timer(timer)
		, m_delay(delay)
	{
		//Turn off immediately
		m_enable = 0;
	}

	virtual bool TurnOn() override
	{
		g_log("Turning on %s\n", m_name);
		m_enable = 1;
		m_timer.Sleep(m_delay);
		return true;
	};

	virtual void TurnOff() override
	{ m_enable = 0; };

protected:
	GPIOPin& m_enable;
	Timer& m_timer;
	uint16_t m_delay;
};

/**
	@brief A power rail that has an active-high enable line and an active-high PGOOD line
 */
class RailDescriptorWithEnableAndPGood : public RailDescriptorWithEnable
{
public:
	/**
		@brief Creates a new rail descriptor

		@param name		Human readable rail name for logging
		@param enable	Enable pin (set high to turn on power)
		@param pgood	Enable pin (set high to turn on power)
		@param timer	Timer to use for sequencing delays
		@param timeout	Timeout, in timer ticks, at which point the rail is expected to have come up
	 */
	RailDescriptorWithEnableAndPGood(const char* name, GPIOPin& enable, GPIOPin& pgood, Timer& timer, uint16_t timeout)
		: RailDescriptorWithEnable(name, enable, timer, timeout)
		, m_pgood(pgood)
	{
		//Turn off immediately
		m_enable = 0;
	}

	virtual bool TurnOn() override
	{
		g_log("Turning on %s\n", m_name);

		m_enable = 1;

		for(uint32_t i=0; i<m_delay; i++)
		{
			if(m_pgood)
				return true;
			m_timer.Sleep(1);
		}

		if(!m_pgood)
		{
			g_log(Logger::ERROR, "Rail %s failed to come up", m_name);
			return false;
		}
		return true;
	};

protected:
	GPIOPin& m_pgood;
};

#endif
