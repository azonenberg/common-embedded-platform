/***********************************************************************************************************************
*                                                                                                                      *
* common-embedded-platform                                                                                             *
*                                                                                                                      *
* Copyright (c) 2024 Andrew D. Zonenberg and contributors                                                              *
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
#include <stm32.h>
#include <peripheral/RTC.h>
#include <staticnet-config.h>
#include <staticnet/stack/staticnet.h>
#include "STM32NTPClient.h"

///@brief KVS key for NTP enable state
static const char* g_ntpEnableObjectID = "ntp.enable";

///@brief KVS key for NTP server IP
static const char* g_ntpServerObjectID = "ntp.server";

///@brief KVS key for NTP UTC offset
static const char* g_ntpUtcOffsetObjectID = "ntp.tzoffset";

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

STM32NTPClient::STM32NTPClient(UDPProtocol* udp)
	: NTPClient(udp)
	, m_initialSyncDone(false)
{
	LoadConfigFromKVS();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BSP interfacing

uint64_t STM32NTPClient::GetLocalTimestamp()
{
	//TODO: use the RTC or a dedicated timer
	//for now just use the log timer
	uint64_t tnow = g_logTimer.GetCount();

	//RTC is 100us steps, convert to native NTP units (2^-32 sec)
	return tnow * 429497;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Notification handlers from NTP stack

void STM32NTPClient::OnTimeUpdated(time_t sec, uint32_t frac)
{
	sec += m_utcOffset;

	//Crack fields to something suitable for feeding to the RTC
	//We can't use newlib's localtime() or localtime_r() because it calls sbrk() under the hood :(
	struct tm cracked;
	if(nullptr == gmtime_r(&sec, &cracked))
		return;

	//Get the OLD RTC time (before applying the NTP shift)
	tm rtctime;
	uint16_t rtcsubsec;
	RTC::GetTime(rtctime, rtcsubsec);

	//Convert sub-second units to 10 kHz tick values (100us)
	uint32_t microseconds = frac / 4295;
	uint16_t subsec = microseconds / 100;

	//Push to the RTC
	uint32_t rtcTicksPerSec = 10000;
	RTC::SetPrescaleAndTime(50, rtcTicksPerSec, cracked, subsec);

	//Calculate the delta between the two timestamps
	int32_t dfrac = rtcsubsec - subsec;
	int32_t dsec =
		rtctime.tm_sec - cracked.tm_sec +
		(rtctime.tm_min - cracked.tm_min) * 60 +
		(rtctime.tm_hour - cracked.tm_hour) * 3600 +
		(rtctime.tm_mday - cracked.tm_mday) * 86400;

	//If fractional delta is negative, normalize it
	if(dfrac < 0)
	{
		dsec --;
		dfrac += rtcTicksPerSec;
	}

	int64_t pollPeriodTicks = m_timeout * rtcTicksPerSec;
	int64_t error = dsec * rtcTicksPerSec + dfrac;
	int64_t ppbError = (1000 * 1000 * 1000) * error / pollPeriodTicks;

	int32_t ppm = ppbError / 1000;
	int32_t ppb = ppbError % 1000;

	//We're now synchronized!
	if(m_initialSyncDone)
	{
		//Log absolute error assuming same polling interval
		g_log("NTP resync complete, local clock error %d.%04d sec over %d sec (%d.%03d ppm)\n",
			dsec, dfrac, m_timeout, ppm, ppb);
	}
	else
	{
		g_log("Initial NTP sync successful, step = %d.%04d sec\n", -dsec, dfrac);
		m_initialSyncDone = true;
	}

	m_lastSync = cracked;
	m_lastSyncFrac = subsec;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Serialization

void STM32NTPClient::LoadConfigFromKVS()
{
	//Check if we're using NTP and, if so, enable it
	bool enable = g_kvs->ReadObject(g_ntpEnableObjectID, /*false*/true);
	if(enable)
		Enable();
	else
		Disable();

	//Load server IP address
	m_serverAddress = g_kvs->ReadObject<IPv4Address>(g_defaultNtpServer, g_ntpServerObjectID);

	//Load UTC offset
	m_utcOffset = g_kvs->ReadObject<int64_t>(-8*3600, g_ntpUtcOffsetObjectID);
}

void STM32NTPClient::SaveConfigToKVS()
{
	if(!g_kvs->StoreObjectIfNecessary(g_ntpEnableObjectID, IsEnabled(), false))
		g_log(Logger::ERROR, "KVS write error\n");

	if(!g_kvs->StoreObjectIfNecessary<IPv4Address>(m_serverAddress, g_defaultNtpServer, g_ntpServerObjectID))
		g_log(Logger::ERROR, "KVS write error\n");

	if(!g_kvs->StoreObjectIfNecessary<int64_t>(m_utcOffset, -8*3600, g_ntpUtcOffsetObjectID))
		g_log(Logger::ERROR, "KVS write error\n");
}
