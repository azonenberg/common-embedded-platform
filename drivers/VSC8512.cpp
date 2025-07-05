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
#include "VSC8512.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

/**
	@brief Constructs the driver object but does not do any PHY initialization

	(this allows the object to be created as a global, but perhaps before the actual MDIO interface has been brought up)
 */
VSC8512::VSC8512(volatile APB_MDIO* mdio, uint8_t baseaddr)
{
	//Initialize the MDIO devices
	for(int i=0; i<12; i++)
		m_mdioDevices[i].DeferredInit(mdio, baseaddr + i);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization

/**
	@brief Verify we are talking to a silicon rev D version of the VSC8512

	@return true on success, false on failure
 */
bool VSC8512::IDCheck()
{
	//Initial ID check
	auto phyid1 = m_mdioDevices[0].ReadRegister(REG_PHY_ID_1);
	auto phyid2 = m_mdioDevices[0].ReadRegister(REG_PHY_ID_2);
	uint8_t stepping = phyid2 & 0xf;
	if( (phyid1 == 0x0007) && (phyid2 >> 4) == 0x06e)
		g_log("PHY ID = %04x %04x (VSC8512 rev %d / stepping %c)\n", phyid1, phyid2, stepping, stepping + 'A');
	else
	{
		g_log("PHY ID = %04x %04x (unknown)\n", phyid1, phyid2);
		return false;
	}

	//Make sure we're a rev D (3) PHY.
	//This part has been out for a while and we shouldn't ever have to deal with older silicon revs
	//given the component shortage clearing out old inventory!
	if( (phyid2 & 0xf) != 3)
	{
		g_log(Logger::ERROR, "Don't know how to initialize silicon revisions other than D\n");
		return false;
	}

	return true;
}

/**
	@brief Reads and writes the AN_ADVERT register with a LFSR to verify MDIO is working properly

	Low level hardware debug routine, not used during normal operation

	@return true on success, false on failure
 */
bool VSC8512::MDIOBitErrorRateCheck()
{
	//Check MDIO signal quality
	g_log("MDIO loopback test\n");
	uint32_t prng = 1;
	for(int i=0; i<500; i++)
	{
		//glibc rand() LFSR, grab low 16 bits and mask off reserved bit
		uint16_t random = (prng & 0xffff) & 0xb000;
		prng = ( (prng * 1103515245) + 12345 ) & 0x7fffffff;

		m_mdioDevices[0].WriteRegister(REG_AN_ADVERT, random);
		uint16_t readback = m_mdioDevices[0].ReadRegister(REG_AN_ADVERT);
		if(readback != random)
		{
			LogIndenter li2(g_log);
			g_log(Logger::ERROR, "MDIO loopback failed (iteration %d): wrote %04x, read %04x\n", i, random, readback);
			return false;
		}
	}

	return true;
}

/**
	@brief Sends a bunch of undocumented register writes extracted from luton26_atom12_revCD_init_script in Microchip MESA

	Presumably chicken bits to make various stuff work better but not thoroughly investigated.

	Does *not* update the 8051 microcode as according to comments, the micro patch for rev D is only needed to work
	around a silicon errata related to 100baseFX mode on the SERDES1G ports. Since we are only using SERDES6G for QSGMII
	and the copper PHYs, this doesn't impact us.

	For now, assumes we are not doing EEE. There's separate chicken bits needed for that.

	@return true on success, false on failure
 */
bool VSC8512::MagicInitScript()
{
	//Initialization script based on values from luton26_atom12_revCD_init_script() in Microchip MESA
	g_log("Running Atom12 rev C/D init script\n");
	m_mdioDevices[0].WriteRegister(REG_VSC8512_PAGESEL, VSC_PAGE_MAIN);
	m_mdioDevices[0].WriteMasked(REG_VSC8512_EXT_CTRL_STAT, 0x0001, 0x0001);	//this register is documented as read only, table 34
	m_mdioDevices[0].WriteRegister(REG_VSC8512_EXT_PHY_CTRL_2, 0x0040);			//+2 edge rate for 100baseTX
																	//Reserved bit 6 set
																	//No jumbo frame support or loopback
	m_mdioDevices[0].WriteRegister(REG_VSC8512_PAGESEL, VSC_PAGE_EXT2);
	m_mdioDevices[0].WriteRegister(VSC_CU_PMD_TX, 0x02be);					//Non-default trim values for 10baseT amplitude

	m_mdioDevices[0].WriteRegister(REG_VSC8512_PAGESEL, VSC_PAGE_TEST);
	m_mdioDevices[0].WriteRegister(20, 0x4420);									//magic undocumented value
	m_mdioDevices[0].WriteRegister(24, 0x0c00);									//magic undocumented value
	m_mdioDevices[0].WriteRegister(9, 0x18c8);									//magic undocumented value
	m_mdioDevices[0].WriteMasked(8, 0x8000, 0x8000);							//magic undocumented value
	m_mdioDevices[0].WriteRegister(5, 0x1320);									//magic undocumented value

	//Magic block of writes to registers 18, 17, 16
	//wtf is token ring even doing in this chipset? or is this misnamed?
	m_mdioDevices[0].WriteRegister(REG_VSC8512_PAGESEL, VSC_PAGE_TR);
	static const uint16_t magicTokenRingBlock[45][3] =
	{
		{ 0x0027, 0x303d, 0x9792 },
		{ 0x00a0, 0xf147, 0x97a0 },
		{ 0x0005, 0x2f54, 0x8fe4 },
		{ 0x0004, 0x01bd, 0x8fae },
		{ 0x000f, 0x000f, 0x8fac },
		{ 0x0000, 0x0004, 0x87fe },
		{ 0x0006, 0x0150, 0x8fe0 },
		{ 0x0012, 0x480a, 0x8f82 },
		{ 0x0000, 0x0034, 0x8f80 },
		{ 0x0000, 0x0012, 0x82e0 },
		{ 0x0005, 0x0208, 0x83a2 },
		{ 0x0000, 0x9186, 0x83b2 },
		{ 0x000e, 0x3700, 0x8fb0 },
		{ 0x0004, 0x9fa1, 0x9688 },
		{ 0x0000, 0xffff, 0x8fd2 },
		{ 0x0003, 0x9fa0, 0x968a },
		{ 0x0020, 0x640b, 0x9690 },
		{ 0x0000, 0x2220, 0x8258 },
		{ 0x0000, 0x2a20, 0x825a },
		{ 0x0000, 0x3060, 0x825c },
		{ 0x0000, 0x3fa0, 0x825e },
		{ 0x0000, 0xe0f0, 0x83a6 },
		{ 0x0000, 0x4489, 0x8f92 },
		{ 0x0000, 0x7000, 0x96a2 },
		{ 0x0010, 0x2048, 0x96a6 },
		{ 0x00ff, 0x0000, 0x96a0 },
		{ 0x0091, 0x9880, 0x8fe8 },
		{ 0x0004, 0xd602, 0x8fea },
		{ 0x00ef, 0xef00, 0x96b0 },
		{ 0x0000, 0x7100, 0x96b2 },
		{ 0x0000, 0x5064, 0x96b4 },
		{ 0x0050, 0x100f, 0x87fa },

		//this block is apparently for regular 10baseT mode
		//need different sequence for 10base-Te
		{ 0x0071, 0xf6d9, 0x8488 },
		{ 0x0000, 0x0db6, 0x848e },
		{ 0x0059, 0x6596, 0x849c },
		{ 0x0000, 0x0514, 0x849e },
		{ 0x0041, 0x0280, 0x84a2 },
		{ 0x0000, 0x0000, 0x84a4 },
		{ 0x0000, 0x0000, 0x84a6 },
		{ 0x0000, 0x0000, 0x84a8 },
		{ 0x0000, 0x0000, 0x84aa },
		{ 0x007d, 0xf7dd, 0x84ae },
		{ 0x006d, 0x95d4, 0x84b0 },
		{ 0x0049, 0x2410, 0x84b2 },

		//apparently this improves 100base-TX link startup
		{ 0x0068, 0x8980, 0x8f90 }
	};

	for(int i=0; i<45; i++)
	{
		m_mdioDevices[0].WriteRegister(18, magicTokenRingBlock[i][0]);
		m_mdioDevices[0].WriteRegister(17, magicTokenRingBlock[i][1]);
		m_mdioDevices[0].WriteRegister(16, magicTokenRingBlock[i][2]);
	}

	m_mdioDevices[0].WriteRegister(REG_VSC8512_PAGESEL, VSC_PAGE_TEST);	//undocumented
	m_mdioDevices[0].WriteMasked(8, 0x0000, 0x8000);

	m_mdioDevices[0].WriteRegister(REG_VSC8512_PAGESEL, VSC_PAGE_MAIN);
	m_mdioDevices[0].WriteMasked(REG_VSC8512_EXT_CTRL_STAT, 0x0000, 0x0001);	//this register is documented as read only, table 34

	//TODO: 8051 microcode patch luton26_atom12_revC_patch?

	return true;
}

/**
	@brief Standard initialization for a single port

	@return true on success, false on failure
 */
bool VSC8512::DefaultPortInit(uint8_t port)
{
	MDIODevice& pdev = m_mdioDevices[port];

	pdev.WriteRegister(REG_VSC8512_PAGESEL, VSC_PAGE_EXT2);
	pdev.WriteRegister(VSC_CU_PMD_TX, 0x02be);							//Non-default trim values for 10baseT amplitude

	pdev.WriteRegister(REG_VSC8512_PAGESEL, VSC_PAGE_EXT3);
	pdev.WriteRegister(VSC_MAC_PCS_CTL, 0x4180);						//Restart MAC on link state change
																		//Default preamble mode
																		//Enable SGMII autonegotiation

	pdev.WriteRegister(REG_VSC8512_PAGESEL, VSC_PAGE_MAIN);

	pdev.WriteRegister(REG_GIG_CONTROL, 0x600);							//Advertise multi-port device, 1000/full
	pdev.WriteRegister(REG_AN_ADVERT, 0x141);							//Advertise 100/full, 10/full only

	pdev.WriteRegister(REG_VSC8512_PAGESEL, VSC_PAGE_MAIN);
	pdev.WriteRegister(REG_VSC8512_LED_MODE, 0x80e0);					//LED configuration (default is 0x8021)
																		//LED3 (not used): half duplex mode
																		//LED2 (not used): link/activity
																		//LED1: constant off
																		//LED0: link state with pulse-stretched activity
	return true;
}

/**
	@brief Configure the undocumented internal temperature sensor

	This apparently uses the internal 8051 and an undocumented ADC to read the thermal diode?

	@return true on success, false on failure
 */
bool VSC8512::InitTempSensor()
{
	//Initialize undocumented temperature sensor (see vtss_phy_chip_temp_init_private in MESA)
	m_mdioDevices[0].WriteRegister(REG_VSC8512_PAGESEL, VSC_PAGE_GENERAL_PURPOSE);
	m_mdioDevices[0].WriteMasked(REG_VSC_TEMP_CONF, 0xc0, 0xc0);
	int16_t tempval = GetTemperature();
	g_log("PHY die temperature: %d.%02d C\n",
		(tempval >> 8),
		static_cast<int>(((tempval & 0xff) / 256.0) * 100));
	return true;
}

/**
	@brief Run some basic sanity checks on a port (doesn't actually pass any traffic)

	@return true on success, false on failure
 */
bool VSC8512::PortHealthCheck(uint8_t port)
{
	//Read the PHY ID
	SelectMainPage();
	auto xphyid1 = m_mdioDevices[port].ReadRegister(REG_PHY_ID_1);
	auto xphyid2 = m_mdioDevices[port].ReadRegister(REG_PHY_ID_2);

	//g_log("PHY ID %d = %04x %04x\n", i, phyid1, phyid2);
	if( (xphyid1 != 0x0007) || (xphyid2 >> 4) != 0x06e)
	{
		g_log(Logger::ERROR, "Port %d health check failed: PHY ID = %04x %04x (invalid)\n",
			(int)port, xphyid1, xphyid2);
		return false;
	}

	return true;
}

/**
	@brief Initialize the PHY

	@return true on success, false on failure
 */
bool VSC8512::Init()
{
	g_log("[new] Initializing PHY at MDIO base address %d\n", m_mdioDevices[0].GetAddress());
	LogIndenter li(g_log);

	//Initial hardware sanity checking
	if(!IDCheck())
		return false;
	//if(!MDIOBitErrorRateCheck())
	//	return false;

	//Global PHY init
	if(!MagicInitScript())
		return false;
	if(!SelectQSGMIIToBaseTMode())
		return false;

	//Per-port init
	for(uint8_t i=0; i<12; i++)
	{
		if(!DefaultPortInit(i))
			return false;

		//TODO: Loopback test??
	}

	//Bring up the undocumented temperature sensor
	InitTempSensor();

	//Sanity check each port to make sure they're responding like we expect
	for(uint8_t i=0; i<12; i++)
	{
		if(!PortHealthCheck(i))
			return false;
	}

	SelectMainPage();

	g_log("PHY init done\n");
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Operating mode and configuration tweaks

/**
	@brief Set the PHY to 12 lanes of QSGMII to 10/100/1000baseT

	@return true on success, false on failure
 */
bool VSC8512::SelectQSGMIIToBaseTMode()
{
	//Ask MCU to set the SERDES operating mode to 12 PHYs with QSGMII (see datasheet table 77)
	//Wait until this completes
	g_log("Selecting 12-PHY QSGMII mode\n");
	PrepareForMCUAccess();
	SendCommandToMCU(0x80a0);

	//Set MAC mode to QSGMII
	m_mdioDevices[0].WriteMasked(REG_VSC_MAC_MODE, 0x0000, 0xc000);
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SERDES configuration

/**
	@brief Set the ob_post0 TX FFE tap on the SERDES6G output buffer
 */
void VSC8512::SetSerdes6GPostCursor0Tap(uint8_t tap)
{
	//Read MCB registers from QSGMII lane 0 as a baseline
	PrepareForMCUAccess();
	ReadMCBToShadowRegisters(1, 1);		//MCB bus 1, macro 1

	//ob_post0 is split between bytes 9 and 10 of cfg_buf so need two read-modify-write transactions

	//Low byte, bits 7:5 are LSBs of tap
	SetIndirectAccessPointer(0x47d8);	//cfg_buf[9] / cfg_vec[79:72]
	uint8_t tmp = MCUPeekByte() & 0x1f;
	tmp |= (tap & 7) << 5;
	MCUPokeByte(tmp, true);				//now pointing to cfg_buf[10]  / cfg_vec[87:80]

	//High byte, bits 2:0 are MSBs of tap
	tmp = MCUPeekByte() & 0xf8;
	tmp |= (tap >> 3) & 7;
	MCUPokeByte(tmp);

	//Push config back to the SERDES
	SetIndirectAccessPointer(0x47ce);	//addr_vec
	MCUPokeByte(0x0e);					//bitmask for SERDES6G lanes 3:1 (all QSGMII lanes)
	PushShadowRegistersToMCB();

	SelectMainPage();					//done, back to main register page
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sensors

/**
	@brief Get the temperature of in 8.8 fixed point format

	(see vtss_phy_read_temp_reg, vtss_phy_chip_temp_get_private)

	Transfer function: adc value * -0.714C + 135.3
 */
uint16_t VSC8512::GetTemperature()
{
	m_mdioDevices[0].WriteRegister(REG_VSC8512_PAGESEL, VSC_PAGE_GENERAL_PURPOSE);

	m_mdioDevices[0].WriteMasked(REG_VSC_TEMP_CONF, 0x00, 0x40);
	m_mdioDevices[0].WriteMasked(REG_VSC_TEMP_CONF, 0x40, 0x40);
	uint8_t tempadc = m_mdioDevices[0].ReadRegister(REG_VSC_TEMP_VAL) & 0xff;
	int16_t tempval = 34636 - 183*tempadc;

	SelectMainPage();
	return tempval;
}
