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

#ifndef TCA6424A_h
#define TCA6424A_h

/**
	@brief Wrapper for TCA6242A I/O expander
 */
class TCA6424A
{
public:
	TCA6424A(I2C* i2c, uint8_t addr);

	void SetDirection(uint8_t chan, bool input);
	void SetOutputValue(uint8_t chan, bool value);

	void BatchUpdateValue(uint8_t block, uint8_t value)
	{ m_outvals[block] = value; };

	void BatchCommitValue();

protected:

	///@brief The I2C channel to sue
	I2C* m_i2c;

	///@brief Device I2C bus address
	uint8_t m_address;

	///@brief Port directions
	uint8_t m_dirmask[3];

	///@brief Output port values
	uint8_t m_outvals[3];
};

/**
	@brief GPIOPin esque wrapper for TCA6424A GPIO channels
 */
class TCA6424A_GPIO
{
public:
	TCA6424A_GPIO(TCA6424A& parent, uint8_t channel)
		: m_parent(parent)
		, m_channel(channel)
		, m_output(false)
		, m_outputValue(false)
	{}

	void SetDirection(bool input)
	{
		m_parent.SetDirection(m_channel, input);
		m_output = !input;
	}

	void operator=(bool value)
	{
		m_parent.SetOutputValue(m_channel, value);
		m_outputValue = value;
	}

	operator bool()
	{
		if(m_output)
			return m_outputValue;

		//FIXME implement input code path
		else
			return false;
	}

protected:
	TCA6424A& m_parent;

	uint8_t m_channel;

	//Cached values for output readback
	bool m_output;

	//Most recently written value
	bool m_outputValue;
};

#endif
