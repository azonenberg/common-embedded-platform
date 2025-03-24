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

#include <core/platform.h>
#include <peripheral/I2C.h>
#include "TCA6424A.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

TCA6424A::TCA6424A(I2C* i2c, uint8_t addr)
	: m_i2c(i2c)
	, m_address(addr)
{
	for(int i=0; i<3; i++)
	{
		//All ports default to input at power up
		m_dirmask[i] = 0xff;

		//All ports default to outputting 1 at power up
		m_outvals[i] = 0xff;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I/O interface logic

/**
	@brief Configure the direction of an output
 */
void TCA6424A::SetDirection(uint8_t chan, bool input)
{
	//Byte group within the expander
	uint8_t group = (chan / 8);

	//Bit lane within the byte
	uint8_t lane = chan % 8;

	//Update internal direction mask
	if(input)
		m_dirmask[group] |= (1 << lane);
	else
		m_dirmask[group] &= ~(1 << lane);

	//Send to the device
	uint8_t regid = 0x0c + group;
	uint8_t data[2] = {regid, m_dirmask[group]};
	m_i2c->BlockingWrite(m_address, data, 2);
}

/**
	@brief Configure the value of an output
 */
void TCA6424A::SetOutputValue(uint8_t chan, bool value)
{
	//Byte group within the expander
	uint8_t group = (chan / 8);

	//Bit lane within the byte
	uint8_t lane = chan % 8;

	//Update internal direction mask
	if(value)
		m_outvals[group] |= (1 << lane);
	else
		m_outvals[group] &= ~(1 << lane);

	//Send to the device
	uint8_t regid = 0x04 + group;
	uint8_t data[2] = {regid, m_outvals[group]};
	m_i2c->BlockingWrite(m_address, data, 2);
}

/**
	@brief Push updates to the expander
 */
void TCA6424A::BatchCommitValue()
{
	//Send to the device
	for(int block=0; block<3; block++)
	{
		uint8_t regid = 0x04 + block;
		uint8_t data[2] = {regid, m_outvals[block]};
		m_i2c->BlockingWrite(m_address, data, 2);
	}
}
