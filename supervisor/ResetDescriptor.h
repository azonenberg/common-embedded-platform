/***********************************************************************************************************************
*                                                                                                                      *
* common-embedded-platform                                                                                             *
*                                                                                                                      *
* Copyright (c) 2023-2026 Andrew D. Zonenberg and contributors                                                         *
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

class ResetDescriptorBase
{
public:
	ResetDescriptorBase(const char* name)
	: m_name(name)
	{}

	virtual void Assert() =0;
	virtual void Deassert() =0;
	virtual bool IsReady() =0;

	const char* GetName() const
	{ return m_name; }

protected:
	const char* m_name;
};

/**
	@brief Wrapper for a reset pin which may be active high or low
 */
template<class T = GPIOPin>
class ResetDescriptor : public ResetDescriptorBase
{
public:
	ResetDescriptor(T& pin, const char* name)
	: ResetDescriptorBase(name)
	, m_pin(pin)
	{}

	virtual bool IsReady()
	{ return true; }


protected:
	T& m_pin;
};

/**
	@brief An active-low reset
 */
template<class T = GPIOPin>
class ActiveLowResetDescriptor : public ResetDescriptor<T>
{
public:
	ActiveLowResetDescriptor(T& pin, const char* name)
	: ResetDescriptor<T> (pin, name)
	{
		//Turn off immediately if it's a regular GPIO pin
		//don't use Assert() since it logs
		//and we might not have the logger set up yet in global constructors
		if(std::is_same_v<T, GPIOPin>)
			ResetDescriptor<T>::m_pin = 0;
	}

	virtual void Assert() override
	{
		g_log("Asserting %s reset\n", ResetDescriptor<T>::m_name);
		ResetDescriptor<T>::m_pin = 0;
	}

	virtual void Deassert() override
	{
		g_log("Releasing %s reset\n", ResetDescriptor<T>::m_name);
		ResetDescriptor<T>::m_pin = 1;
	}
};

/**
	@brief An active-low reset with a time delay
 */
template<class T = GPIOPin>
class ActiveLowResetDescriptorWithDelay : public ActiveLowResetDescriptor<T>
{
public:
	ActiveLowResetDescriptorWithDelay(T& pin, const char* name, Timer& timer, uint16_t delay)
	: ActiveLowResetDescriptor<T>(pin, name)
	, m_timer(timer)
	, m_delay(delay)
	, m_done(false)
	{

	}

	virtual void Deassert() override
	{
		g_log("Releasing %s reset\n", ResetDescriptor<T>::m_name);
		ResetDescriptor<T>::m_pin = 1;
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
template<class T = GPIOPin>
class ActiveLowResetDescriptorWithActiveHighDone : public ActiveLowResetDescriptor<T>
{
public:
	ActiveLowResetDescriptorWithActiveHighDone(T& rst, T& done, const char* name)
	: ActiveLowResetDescriptor<T>(rst, name)
	, m_done(done)
	{}

	virtual bool IsReady() override
	{ return m_done; }

protected:
	T& m_done;
};

/**
	@brief An active-low reset plus an active-low signal that's asserted when the device is operational
 */
template<class T = GPIOPin>
class ActiveLowResetDescriptorWithActiveLowDone : public ActiveLowResetDescriptor<T>
{
public:
	ActiveLowResetDescriptorWithActiveLowDone(T& rst, T& done, const char* name)
	: ActiveLowResetDescriptor<T>(rst, name)
	, m_done(done)
	{}

	virtual bool IsReady() override
	{ return !m_done; }

protected:
	T& m_done;
};

/**
	@brief An active-high reset
 */
template<class T = GPIOPin>
class ActiveHighResetDescriptor : public ResetDescriptor<T>
{
public:
	ActiveHighResetDescriptor(T& pin, const char* name)
	: ResetDescriptor<T>(pin, name)
	{
		//Turn off immediately if it's a regular GPIO pin
		//don't use Assert() since it logs
		//and we might not have the logger set up yet in global constructors
		if(std::is_same_v<T, GPIOPin>)
			ResetDescriptor<T>::m_pin = 1;
	}

	virtual void Assert() override
	{
		g_log("Asserting %s reset\n", ResetDescriptor<T>::m_name);
		ResetDescriptor<T>::m_pin = 1;
	}

	virtual void Deassert() override
	{
		g_log("Releasing %s reset\n", ResetDescriptor<T>::m_name);
		ResetDescriptor<T>::m_pin = 0;
	}
};

/**
	@brief An active-low reset with a time delay
 */
template<class T = GPIOPin>
class ActiveHighResetDescriptorWithDelay : public ActiveHighResetDescriptor<T>
{
public:
	ActiveHighResetDescriptorWithDelay(T& pin, const char* name, Timer& timer, uint16_t delay)
	: ActiveHighResetDescriptor<T>(pin, name)
	, m_timer(timer)
	, m_delay(delay)
	, m_done(false)
	{

	}

	virtual void Deassert() override
	{
		g_log("Releasing %s reset\n", ResetDescriptor<T>::m_name);
		ResetDescriptor<T>::m_pin = 0;
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
