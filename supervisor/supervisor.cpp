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

#include "supervisor-common.h"
#include "IBCRegisterReader.h"
#include "TempSensorReader.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals

//Addresses for power supply stuff on the management I2C bus
const uint8_t g_tempI2cAddress = 0x90;
const uint8_t g_ibcI2cAddress = 0x42;

#ifdef HAVE_ADC
///@brief The ADC (can't be initialized before InitClocks() so can't be a global object)
ADC* g_adc = nullptr;
#endif

///@brief Chip select pin for our SPI peripheral
GPIOPin* g_spiCS = nullptr;

//IBC version strings
char g_ibcSwVersion[20] = {0};
char g_ibcHwVersion[20] = {0};

///@brief Our firmware version string
char g_version[20] = {0};

//Current IBC sensor readings
uint16_t g_ibcTemp = 0;
uint16_t g_ibc3v3 = 0;
uint16_t g_ibcMcuTemp = 0;
uint16_t g_vin48 = 0;
uint16_t g_vout12 = 0;
uint16_t g_voutsense = 0;
uint16_t g_iin = 0;
uint16_t g_iout = 0;
uint16_t g_3v3Voltage = 0;
uint16_t g_mcutemp = 0;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Hardware initialization

void Super_Init()
{
	Super_InitI2C();
	Super_InitIBC();

	#ifdef HAVE_ADC
	Super_InitADC();
	#endif
}

void Super_InitI2C()
{
	g_log("Initializing IBC I2C interface\n");

	//Initialize the I2C then wait a bit longer
	//(i2c pin states prior to init are unknown)
	g_logTimer.Sleep(100);
	g_i2c.Reset();
	g_logTimer.Sleep(100);

	//Set temperature sensor to max resolution
	//(if it doesn't respond, the i2c is derped out so reset and try again)
	for(int i=0; i<5; i++)
	{
		uint8_t cmd[3] = {0x01, 0x60, 0x00};
		if(g_i2c.BlockingWrite(g_tempI2cAddress, cmd, sizeof(cmd)))
			break;

		g_log(
			Logger::WARNING,
			"Failed to initialize I2C temp sensor at 0x%02x, resetting and trying again\n",
			g_tempI2cAddress);

		g_i2c.Reset();
		g_logTimer.Sleep(100);
	}
}

void Super_InitIBC()
{
	g_log("Connecting to IBC\n");
	LogIndenter li(g_log);

	//Wait a while to make sure the IBC is booted before we come up
	//(both us and the IBC come up off 3V3_SB as soon as it's up, with no sequencing)
	g_logTimer.Sleep(2500);

	g_i2c.BlockingWrite8(g_ibcI2cAddress, IBC_REG_VERSION);
	g_i2c.BlockingRead(g_ibcI2cAddress, (uint8_t*)g_ibcSwVersion, sizeof(g_ibcSwVersion));
	g_log("IBC firmware version %s\n", g_ibcSwVersion);

	g_i2c.BlockingWrite8(g_ibcI2cAddress, IBC_REG_HW_VERSION);
	g_i2c.BlockingRead(g_ibcI2cAddress, (uint8_t*)g_ibcHwVersion, sizeof(g_ibcHwVersion));
	g_log("IBC hardware version %s\n", g_ibcHwVersion);
}

#ifdef HAVE_ADC
void Super_InitADC()
{
	g_log("Initializing ADC\n");
	LogIndenter li(g_log);

	//Run ADC at sysclk/10 (10 MHz)
	static ADC adc(&_ADC, &_ADC.chans[0], 10);
	g_adc = &adc;
	g_logTimer.Sleep(20);

	//Set up sampling time. Need minimum 5us to accurately read temperature
	//With ADC clock of 8 MHz = 125 ns per cycle this is 40 cycles
	//Max 8 us / 64 clocks for input channels
	//47.5 clocks fits both requirements, use it for everything
	int tsample = 95;
	for(int i=0; i <= 18; i++)
		adc.SetSampleTime(tsample, i);
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IBC sensor interfacing

/**
	@brief Requests more sensor data from the IBC

	@return true if sensor values are updated
 */
bool PollIBCSensors()
{
	static IBCRegisterReader regreader;
	static TempSensorReader tempreader(g_i2c, g_tempI2cAddress);

	static int state = 0;

	static int iLastUpdate = 0;
	iLastUpdate ++;

	if(iLastUpdate > 30000)
	{
		g_log(Logger::WARNING, "I2C sensor state machine timeout (IBC hang or reboot?), resetting and trying again\n");

		//Reset both readers and return to the idle state, wait 10ms before retrying anything
		tempreader.Reset();
		regreader.Reset();
		g_i2c.Reset();
		state = 0;
		iLastUpdate = 0;
		g_logTimer.Sleep(2);
	}

	//Read the values
	switch(state)
	{
		case 0:
			if(tempreader.ReadTempNonblocking(g_ibcTemp))
			{
				iLastUpdate = 0;
				state ++;
			}
			break;

		case 1:
			if(regreader.ReadRegisterNonblocking(IBC_REG_VIN, g_vin48))
			{
				iLastUpdate = 0;
				state ++;
			}
			break;

		case 2:
			if(regreader.ReadRegisterNonblocking(IBC_REG_VOUT, g_vout12))
			{
				iLastUpdate = 0;
				state ++;
			}
			break;

		case 3:
			if(regreader.ReadRegisterNonblocking(IBC_REG_VSENSE, g_voutsense))
			{
				iLastUpdate = 0;
				state ++;
			}
			break;

		case 4:
			if(regreader.ReadRegisterNonblocking(IBC_REG_IIN, g_iin))
				state ++;
			break;

		case 5:
			if(regreader.ReadRegisterNonblocking(IBC_REG_IOUT, g_iout))
				state ++;
			break;

		case 6:
			if(regreader.ReadRegisterNonblocking(IBC_REG_MCU_TEMP, g_ibcMcuTemp))
				state ++;
			break;

		case 7:
			if(regreader.ReadRegisterNonblocking(IBC_REG_3V3_SB, g_ibc3v3))
				state ++;
			break;

		#ifdef HAVE_ADC

		//Also read our own internal health sensors at this point in the rotation
		//(should we rename this function PollHealthSensors or something?)
		//TODO: nonblocking ADC accesses?
		case 8:
			if(g_adc->GetTemperatureNonblocking(g_mcutemp))
				state ++;
			break;

		case 9:
			g_3v3Voltage = g_adc->GetSupplyVoltage();
			state ++;
			break;

		#endif

		//end of loop, wrap around
		default:
			state = 0;
			return true;
	}

	return false;
}
