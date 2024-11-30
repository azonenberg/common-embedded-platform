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

#ifndef STM32NTPClient_h
#define STM32NTPClient_h

#include <staticnet/ntp/NTPClient.h>

/**
	@brief NTP client service
 */
class STM32NTPClient : public NTPClient
{
public:
	STM32NTPClient(UDPProtocol* udp);

	void LoadConfigFromKVS();
	void SaveConfigToKVS();

	bool IsSynchronized()
	{ return (m_state != STATE_DESYNCED) && m_initialSyncDone; }

	void GetLastSync(tm& t, uint16_t& frac)
	{
		t = m_lastSync;
		frac = m_lastSyncFrac;
	}

protected:
	virtual uint64_t GetLocalTimestamp();

	virtual void OnTimeUpdated(time_t sec, uint32_t frac);

	///@brief True if we've synced at least once since boot
	bool m_initialSyncDone;

	///@brief Timestamp of the last successful sync
	tm m_lastSync;
	uint16_t m_lastSyncFrac;

	///@brief UTC offset
	int64_t m_utcOffset;
};

#endif
