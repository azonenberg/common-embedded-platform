/***********************************************************************************************************************
*                                                                                                                      *
* common-embedded-platform                                                                                             *
*                                                                                                                      *
* Copyright (c) 2023-2025 Andrew D. Zonenberg and contributors                                                         *
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

#ifndef ResetDescriptor_h
#define ResetDescriptor_h

#include <peripheral/GPIO.h>

/**
	@brief Wrapper for a reset pin which may be active high or low
 */
class ResetDescriptor
{
public:
	ResetDescriptor(GPIOPin& pin, const char* name)
	: m_pin(pin)
	, m_name(name)
	{}

	virtual void Assert() =0;
	virtual void Deassert() =0;

	virtual bool IsReady()
	{ return true; }

	const char* GetName() const
	{ return m_name; }

protected:
	GPIOPin& m_pin;
	const char* m_name;
};

/**
	@brief An active-low reset
 */
class ActiveLowResetDescriptor : public ResetDescriptor
{
public:
	ActiveLowResetDescriptor(GPIOPin& pin, const char* name)
	: ResetDescriptor(pin, name)
	{ m_pin = 0; }	//don't use Assert() since it logs
					//and we might not have the logger set up yet in global constructors

	virtual void Assert() override
	{
		g_log("Asserting %s reset\n", m_name);
		m_pin = 0;
	}

	virtual void Deassert() override
	{
		g_log("Releasing %s reset\n", m_name);
		m_pin = 1;
	}
};

/**
	@brief An active-low reset with a time delay
 */
class ActiveLowResetDescriptorWithDelay : public ActiveLowResetDescriptor
{
public:
	ActiveLowResetDescriptorWithDelay(GPIOPin& pin, const char* name, Timer& timer, uint16_t delay)
	: ActiveLowResetDescriptor(pin, name)
	, m_timer(timer)
	, m_delay(delay)
	, m_done(false)
	{

	}

	virtual void Deassert() override
	{
		g_log("Releasing %s reset\n", m_name);
		m_pin = 1;
		m_done = false;
		m_tstart = m_timer.GetCount();
	}

	virtual bool IsReady() override
	{
		//Immediately report done on wrap
		//TODO: cleaner handling?
		auto tnow = m_timer.GetCount();
		if(tnow < m_tstart)
			m_done = true;

		//Elapsed?
		if(tnow > (m_tstart + m_delay) )
			m_done = true;

		return m_done;
	}

protected:
	Timer& m_timer;
	uint16_t m_delay;

	bool m_done;
	uint32_t m_tstart;
};

/**
	@brief An active-low reset plus an active-high signal that's asserted when the device is operational
 */
class ActiveLowResetDescriptorWithActiveHighDone : public ActiveLowResetDescriptor
{
public:
	ActiveLowResetDescriptorWithActiveHighDone(GPIOPin& rst, GPIOPin& done, const char* name)
	: ActiveLowResetDescriptor(rst, name)
	, m_done(done)
	{}

	virtual bool IsReady() override
	{ return m_done; }

protected:
	GPIOPin& m_done;
};

/**
	@brief An active-low reset plus an active-low signal that's asserted when the device is operational
 */
class ActiveLowResetDescriptorWithActiveLowDone : public ActiveLowResetDescriptor
{
public:
	ActiveLowResetDescriptorWithActiveLowDone(GPIOPin& rst, GPIOPin& done, const char* name)
	: ActiveLowResetDescriptor(rst, name)
	, m_done(done)
	{}

	virtual bool IsReady() override
	{ return !m_done; }

protected:
	GPIOPin& m_done;
};

/**
	@brief An active-high reset
 */
class ActiveHighResetDescriptor : public ResetDescriptor
{
public:
	ActiveHighResetDescriptor(GPIOPin& pin, const char* name)
	: ResetDescriptor(pin, name)
	{ m_pin = 1; }	//don't use Assert() since it logs
					//and we might not have the logger set up yet in global constructors

	virtual void Assert() override
	{
		g_log("Asserting %s reset\n", m_name);
		m_pin = 1;
	}

	virtual void Deassert() override
	{
		g_log("Releasing %s reset\n", m_name);
		m_pin = 0;
	}
};

/**
	@brief An active-low reset with a time delay
 */
class ActiveHighResetDescriptorWithDelay : public ActiveHighResetDescriptor
{
public:
	ActiveHighResetDescriptorWithDelay(GPIOPin& pin, const char* name, Timer& timer, uint16_t delay)
	: ActiveHighResetDescriptor(pin, name)
	, m_timer(timer)
	, m_delay(delay)
	, m_done(false)
	{

	}

	virtual void Deassert() override
	{
		g_log("Releasing %s reset\n", m_name);
		m_pin = 0;
		m_done = false;
		m_tstart = m_timer.GetCount();
	}

	virtual bool IsReady() override
	{
		//Immediately report done on wrap
		//TODO: cleaner handling?
		auto tnow = m_timer.GetCount();
		if(tnow < m_tstart)
			m_done = true;

		//Elapsed?
		if(tnow > (m_tstart + m_delay) )
			m_done = true;

		return m_done;
	}

protected:
	Timer& m_timer;
	uint16_t m_delay;

	bool m_done;
	uint32_t m_tstart;
};

#endif
