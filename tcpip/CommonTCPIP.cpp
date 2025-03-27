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

#include <core/platform.h>
#include "CommonTCPIP.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals

/**
	@brief Mapping of link speed IDs to printable names
 */
const char* g_linkSpeedNamesLong[] =
{
	"10 Mbps",
	"100 Mbps",
	"1000 Mbps",
	"10 Gbps",
	"25 Gbps",
	"40 Gbps",
	"100 Gbps"
};

///@brief Our MAC address
MACAddress g_macAddress;

///@brief Our IPv4 address
IPv4Config g_ipConfig;

///@brief Our IPv6 address
IPv6Config g_ipv6Config;

///@brief Ethernet protocol stack
EthernetProtocol* g_ethProtocol = nullptr;

///@brief BaseT link status
bool g_basetLinkUp = false;

//Ethernet link speed
linkspeed_t g_basetLinkSpeed = LINK_SPEED_10M;

///@brief MDIO device for the PHY
MDIODevice* g_phyMdio = nullptr;

//Default IP config, can override if needed in application
__attribute__((weak)) extern const IPv4Address g_defaultIP			= { .m_octets{ 10,   2,   6,  50} };
__attribute__((weak)) extern const IPv4Address g_defaultNetmask		= { .m_octets{255, 255, 255,   0} };
__attribute__((weak)) extern const IPv4Address g_defaultBroadcast	= { .m_octets{ 10,   2,   6, 255} };
__attribute__((weak)) extern const IPv4Address g_defaultGateway		= { .m_octets{ 10,   2,   6, 252} };
__attribute__((weak)) extern const IPv4Address g_defaultNtpServer	= { .m_octets{ 10,   2,   5,  26} };

void InitMacEEPROM()
{
	g_log("Initializing MAC address EEPROM\n");

	//Extended memory block for MAC address data isn't in the normal 0xa* memory address space
	//uint8_t main_addr = 0xa0;
	uint8_t ext_addr = 0xb0;

	//Pointers within extended memory block
	uint8_t serial_offset = 0x80;
	uint8_t mac_offset = 0x9a;

	//Read MAC address
	g_macI2C.BlockingWrite8(ext_addr, mac_offset);
	g_macI2C.BlockingRead(ext_addr, &g_macAddress[0], sizeof(g_macAddress));

	//Read serial number
	const int serial_len = 16;
	uint8_t serial[serial_len] = {0};
	g_macI2C.BlockingWrite8(ext_addr, serial_offset);
	g_macI2C.BlockingRead(ext_addr, serial, serial_len);

	{
		LogIndenter li(g_log);
		g_log("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
			g_macAddress[0], g_macAddress[1], g_macAddress[2], g_macAddress[3], g_macAddress[4], g_macAddress[5]);

		g_log("EEPROM serial number: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
			serial[0], serial[1], serial[2], serial[3], serial[4], serial[5], serial[6], serial[7],
			serial[8], serial[9], serial[10], serial[11], serial[12], serial[13], serial[14], serial[15]);
	}
}

/**
	@brief Load our IP configuration from the KVS
 */
void ConfigureIP()
{
	g_ipConfig.m_address = g_kvs->ReadObject<IPv4Address>(g_defaultIP, "ip.address");
	g_ipConfig.m_netmask = g_kvs->ReadObject<IPv4Address>(g_defaultNetmask, "ip.netmask");
	g_ipConfig.m_broadcast = g_kvs->ReadObject<IPv4Address>(g_defaultBroadcast, "ip.broadcast");
	g_ipConfig.m_gateway = g_kvs->ReadObject<IPv4Address>(g_defaultGateway, "ip.gateway");

	//TODO
	memset(&g_ipv6Config, 0, sizeof(g_ipv6Config));
}
