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

#ifndef VSC8512_h
#define VSC8512_h

#include <APB_MDIO.h>

enum mdioreg_t_ext
{
	//VSC8512 specific
	REG_VSC8512_PAGESEL			= 0x1f,

	//VSC8512 main/standard page
	REG_VSC8512_EXT_CTRL_STAT	= 0x14,
	REG_VSC8512_EXT_PHY_CTRL_2	= 0x18,
	REG_VSC8512_AUX_CTRL_STAT	= 0x1c,

	//VSC8512 extended page 1
	REG_VSC8512_LED_MODE		= 0x1d,

	//VSC8512 extended page 2
	VSC_CU_PMD_TX				= 0x10,

	//VSC8512 extended page 3
	VSC_MAC_PCS_CTL				= 0x10,

	//GPIO / global command page
	REG_VSC_GP_GLOBAL_SERDES	= 0x12,
	REG_VSC_MAC_MODE			= 0x13,
	//14.2.3 p18 says 19G 15:14 = 00/10
	REG_VSC_TEMP_CONF			= 0x1a,
	REG_VSC_TEMP_VAL			= 0x1c
};

enum vsc_page_t
{
	VSC_PAGE_MAIN				= 0x0000,
	VSC_PAGE_EXT1				= 0x0001,
	VSC_PAGE_EXT2				= 0x0002,
	VSC_PAGE_EXT3				= 0x0003,

	VSC_PAGE_GENERAL_PURPOSE	= 0x0010,
	VSC_PAGE_TEST				= 0x2a30,
	VSC_PAGE_TR					= 0x52b5
};

/**
	@brief Driver for Microchip/Vitesse VSC8512 12-port QSGMII PHY management interface

	See https://www.serd.es/2025/07/04/Switch-project-pt3.html
 */
class VSC8512
{
public:
	VSC8512(volatile APB_MDIO* mdio, uint8_t baseaddr);

	//Initialization
	bool Init();

	//Sensors
	uint16_t GetTemperature();

	//SERDES config
	void SetSerdes6GPostCursor0Tap(uint8_t tap);

protected:
	bool IDCheck();
	bool MDIOBitErrorRateCheck();
	bool MagicInitScript();

	bool SelectQSGMIIToBaseTMode();
	bool DefaultPortInit(uint8_t port);
	bool InitTempSensor();
	bool PortHealthCheck(uint8_t port);

	///@brief Set up the register banking to access the MCU and make sure it's not busy
	void PrepareForMCUAccess()
	{
		m_mdioDevices[0].WriteRegister(REG_VSC8512_PAGESEL, VSC_PAGE_GENERAL_PURPOSE);
		WaitForMCU();
	}

	///@brief Return to the IEEE standard register page
	void SelectMainPage()
	{ m_mdioDevices[0].WriteRegister(REG_VSC8512_PAGESEL, VSC_PAGE_MAIN); }

	/**
		@brief Send a command to the MCU

		Assumes VSC_PAGE_GENERAL_PURPOSE is already selected

		@param cmd	16-bit command value
	 */
	void SendCommandToMCU(uint16_t cmd)
	{
		m_mdioDevices[0].WriteRegister(REG_VSC_GP_GLOBAL_SERDES, cmd | 0x8000);	//set "execute cmd" bit
		WaitForMCU();
	}

	/**
		@brief Wait for the MCU to finish any previous command

		Assumes VSC_PAGE_GENERAL_PURPOSE is already selected
	 */
	void WaitForMCU()
	{
		while((m_mdioDevices[0].ReadRegister(REG_VSC_GP_GLOBAL_SERDES) & 0x8000) != 0)
		{}
	}

	/**
		@brief Read SERDES configuration from MCB to shadow registers
	 */
	void ReadMCBToShadowRegisters(uint8_t mcbBusIndex, uint8_t serdesIndex)
	{ SendCommandToMCU( (serdesIndex << 8) | (mcbBusIndex << 4) | 0x3); }

	/**
		@brief Push SERDES configuration from shadow registers to MCB
	 */
	void PushShadowRegistersToMCB()
	{ SendCommandToMCU(0x9cc0); }

	/**
		@brief Sets the pointer for indirect register access
	 */
	void SetIndirectAccessPointer(uint16_t ptr)
	{
		//flag bit for address space selector or something
		if(ptr & 0x4000)
			ptr |= 0x1000;
		SendCommandToMCU(0x4000 | ptr);
	}

	/**
		@brief Read a byte from the 8051 memory space at the address selected by SetIndirectAccessPointer
	 */
	uint8_t MCUPeekByte(bool postIncrement = false)
	{
		//Send the read command
		if(postIncrement)
			SendCommandToMCU(0x1007);
		else
			SendCommandToMCU(0x0007);

		//Read the reply
		uint8_t ret = m_mdioDevices[0].ReadRegister(REG_VSC_GP_GLOBAL_SERDES);
		return (ret >> 4) & 0xff;
	}

	/**
		@brief Write a byte to the 8051 memory space at the address selected by SetIndirectAccessPointer
	 */
	void MCUPokeByte(uint8_t bval, bool postIncrement = false)
	{
		uint16_t cmd = 0x6;
		if(postIncrement)
			cmd |= 0x1000;
		SendCommandToMCU(cmd | (bval << 4));
	}

	MDIODevice m_mdioDevices[12];
};

#endif
