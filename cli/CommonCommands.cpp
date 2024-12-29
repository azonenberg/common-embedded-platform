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

#include <fpga/AcceleratedCryptoEngine.h>
#include "CommonCommands.h"

/**
	@brief Prints info about the processor
 */
void PrintProcessorInfo(CLIOutputStream* stream)
{
	stream->Printf("MCU:\n");

	uint16_t rev = DBGMCU.IDCODE >> 16;
	uint16_t device = DBGMCU.IDCODE & 0xfff;

	if(device == 0x483)
	{
		//Look up the stepping number
		const char* srev = nullptr;
		switch(rev)
		{
			case 0x1000:
				srev = "A";
				break;

			case 0x1001:
				srev = "Z";
				break;

			default:
				srev = "(unknown)";
		}

		uint8_t pkg = SYSCFG.PKGR;
		const char* package = "";
		switch(pkg)
		{
			case 0:
				package = "VQFPN68 (industrial)";
				break;
			case 1:
				package = "LQFP100/TFBGA100 (legacy)";
				break;
			case 2:
				package = "LQFP100 (industrial)";
				break;
			case 3:
				package = "TFBGA100 (industrial)";
				break;
			case 4:
				package = "WLCSP115 (industrial)";
				break;
			case 5:
				package = "LQFP144 (legacy)";
				break;
			case 6:
				package = "UFBGA144 (legacy)";
				break;
			case 7:
				package = "LQFP144 (industrial)";
				break;
			case 8:
				package = "UFBGA169 (industrial)";
				break;
			case 9:
				package = "UFBGA176+25 (industrial)";
				break;
			case 10:
				package = "LQFP176 (industrial)";
				break;
			default:
				package = "unknown package";
				break;
		}

		stream->Printf("    STM32%c%c%c%c stepping %s, %s\n",
			(L_ID >> 24) & 0xff,
			(L_ID >> 16) & 0xff,
			(L_ID >> 8) & 0xff,
			(L_ID >> 0) & 0xff,
			srev,
			package
			);
		stream->Printf("    564 kB total SRAM, 128 kB DTCM, up to 256 kB ITCM, 4 kB backup SRAM\n");
		stream->Printf("    %d kB Flash\n", F_ID);

		//U_ID fields documented in 45.1 of STM32 programming manual
		uint16_t waferX = U_ID[0] >> 16;
		uint16_t waferY = U_ID[0] & 0xffff;
		uint8_t waferNum = U_ID[1] & 0xff;
		char waferLot[8] =
		{
			static_cast<char>((U_ID[1] >> 24) & 0xff),
			static_cast<char>((U_ID[1] >> 16) & 0xff),
			static_cast<char>((U_ID[1] >> 8) & 0xff),
			static_cast<char>((U_ID[2] >> 24) & 0xff),
			static_cast<char>((U_ID[2] >> 16) & 0xff),
			static_cast<char>((U_ID[2] >> 8) & 0xff),
			static_cast<char>((U_ID[2] >> 0) & 0xff),
			'\0'
		};
		stream->Printf("    Lot %s, wafer %d, die (%d, %d)\n", waferLot, waferNum, waferX, waferY);
	}
	else
		stream->Printf("    Unknown device (0x%06x)\n", device);

	//Print CPU info
	if( (SCB.CPUID & 0xff00fff0) == 0x4100c270 )
	{
		stream->Printf("    ARM Cortex-M7 r%dp%d\n", (SCB.CPUID >> 20) & 0xf, (SCB.CPUID & 0xf));

		//TODO: figure out why this isn't showing on H735, which should have cache
		if(SCB.CLIDR & 2)
		{
			stream->Printf("        L1 data cache present\n");
			SCB.CCSELR = 0;

			int sets = ((SCB.CCSIDR >> 13) & 0x7fff) + 1;
			int ways = ((SCB.CCSIDR >> 3) & 0x3ff) + 1;
			int words = 1 << ((SCB.CCSIDR & 3) + 2);
			int total = (sets * ways * words * 4) / 1024;
			stream->Printf("            %d sets, %d ways, %d words per line, %d kB total\n",
				sets, ways, words, total);
		}
		if(SCB.CLIDR & 1)
		{
			stream->Printf("        L1 instruction cache present\n");
			SCB.CCSELR = 1;

			int sets = ((SCB.CCSIDR >> 13) & 0x7fff) + 1;
			int ways = ((SCB.CCSIDR >> 3) & 0x3ff) + 1;
			int words = 1 << ((SCB.CCSIDR & 3) + 2);
			int total = (sets * ways * words * 4) / 1024;
			stream->Printf("            %d sets, %d ways, %d words per line, %d kB total\n",
				sets, ways, words, total);
		}
	}
	else
		stream->Printf("    Unknown CPU (0x%08x)\n", SCB.CPUID);
}

/**
	@brief Print summary information about the KVS
 */
void PrintFlashSummary(CLIOutputStream* stream)
{
	//Print info about the flash memory in general
	stream->Printf("Flash configuration storage is 2 banks of %d kB\n", g_kvs->GetBlockSize() / 1024);
	if(g_kvs->IsLeftBankActive())
		stream->Printf("    Active bank: Left\n");
	else
		stream->Printf("    Active bank: Right\n");
	stream->Printf("    Header version: %d\n", g_kvs->GetBankHeaderVersion());
	stream->Printf("    Log area:    %6d / %6d entries free (%d %%)\n",
		g_kvs->GetFreeLogEntries(),
		g_kvs->GetLogCapacity(),
		g_kvs->GetFreeLogEntries()*100 / g_kvs->GetLogCapacity());
	stream->Printf("    Data area:   %6d / %6d kB free      (%d %%)\n",
		g_kvs->GetFreeDataSpace() / 1024,
		g_kvs->GetDataCapacity() / 1024,
		g_kvs->GetFreeDataSpace() * 100 / g_kvs->GetDataCapacity());

	//Dump directory listing
	const uint32_t nmax = 256;
	KVSListEntry list[nmax];
	uint32_t nfound = g_kvs->EnumObjects(list, nmax);
	stream->Printf("    Objects:\n");
	stream->Printf("        Key                               Size  Revisions\n");
	int size = 0;
	for(uint32_t i=0; i<nfound; i++)
	{
		//If the object has no content, don't show it (it's been deleted)
		if(list[i].size == 0)
			continue;

		//Is this a group?
		auto dotpos = strchr(list[i].key, '.');
		if(dotpos != nullptr)
		{
			//Get the name of the group
			char groupname[KVS_NAMELEN+1] = {0};
			auto grouplen = dotpos + 1 - list[i].key;
			memcpy(groupname, list[i].key, grouplen);

			//If we have a previous key with the same group, we're not the first
			bool first = true;
			if(i > 0)
			{
				if(memcmp(list[i-1].key, groupname, grouplen) == 0)
					first = false;
			}

			//Do we have a subsequent key with the same group?
			bool next = true;
			if(i+1 < nfound)
			{
				if(memcmp(list[i+1].key, groupname, grouplen) != 0)
					next = false;
			}
			else
				next = false;

			//Trim off the leading dot in the group
			groupname[grouplen-1] = '\0';

			//Beginning of a group (with more than one key)? Add the heading
			if(first && next)
				stream->Printf("        %-32s\n", groupname);

			//If in a group with >1 item, print the actual entry
			if(next || !first)
			{
				//Print the tree node
				if(next)
					stream->Printf("        ├── %-28s %5d  %d\n", list[i].key + grouplen, list[i].size, list[i].revs);
				else
					stream->Printf("        └── %-28s %5d  %d\n", list[i].key + grouplen, list[i].size, list[i].revs);
			}

			//Single entry group, normal print
			else
				stream->Printf("        %-32s %5d  %d\n", list[i].key, list[i].size, list[i].revs);
		}

		//No, not in a group
		else
			stream->Printf("        %-32s %5d  %d\n", list[i].key, list[i].size, list[i].revs);

		//Record total data size
		size += list[i].size;
	}
	stream->Printf("    %d objects total (%d.%02d kB)\n",
		nfound,
		size/1024, (size % 1024) * 100 / 1024);
}

/**
	@brief Print detailed information about a flash object
 */
void PrintFlashDetails(CLIOutputStream* stream, const char* objectName)
{
	auto hlog = g_kvs->FindObject(objectName);
	if(!hlog)
	{
		stream->Printf("Object not found\n");
		return;
	}

	//TODO: show previous versions too?
	stream->Printf("Object \"%s\":\n", objectName);
	{
		stream->Printf("    Start:  0x%08x\n", hlog->m_start);
		stream->Printf("    Length: 0x%08x\n", hlog->m_len);
		stream->Printf("    CRC32:  0x%08x\n", hlog->m_crc);
	}

	auto pdata = g_kvs->MapObject(hlog);

	//TODO: make this a dedicated hexdump routine
	const uint32_t linelen = 16;
	for(uint32_t i=0; i<hlog->m_len; i += linelen)
	{
		stream->Printf("%04x   ", i);

		//Print hex
		for(uint32_t j=0; j<linelen; j++)
		{
			//Pad with spaces so we get good alignment on the end of the block
			if(i+j >= hlog->m_len)
				stream->Printf("   ");

			else
				stream->Printf("%02x ", pdata[i+j]);
		}

		stream->Printf("  ");

		//Print ASCII
		for(uint32_t j=0; j<linelen; j++)
		{
			//No padding needed here
			if(i+j >= hlog->m_len)
				break;

			else if(isprint(pdata[i+j]))
				stream->Printf("%c", pdata[i+j]);
			else
				stream->Printf(".");
		}

		stream->Printf("\n");
	}
}

void RemoveFlashKey(CLIOutputStream* stream, const char* key)
{
	auto hlog = g_kvs->FindObject(key);
	if(hlog)
	{
		if(!g_kvs->StoreObject(key, nullptr, 0))
			stream->Printf("KVS write error\n");
		else
			stream->Printf("Object \"%s\" deleted\n", key);
	}
	else
		stream->Printf("Object \"%s\" not found, could not delete\n", key);
}

void PrintSSHHostKey(CLIOutputStream* stream)
{
	char buf[64] = {0};
	AcceleratedCryptoEngine tmp;
	tmp.GetHostKeyFingerprint(buf, sizeof(buf));
	stream->Printf("ED25519 key fingerprint is SHA256:%s.\n", buf);
}

void PrintARPCache(CLIOutputStream* stream, EthernetProtocol* eth)
{
	auto cache = eth->GetARP()->GetCache();

	uint32_t ways = cache->GetWays();
	uint32_t lines = cache->GetLines();
	stream->Printf("ARP cache is %d ways of %d lines, %d spaces total\n", ways, lines, ways*lines);

	stream->Printf("Expiration  HWaddress           Address\n");

	for(uint32_t i=0; i<ways; i++)
	{
		auto way = cache->GetWay(i);
		for(uint32_t j=0; j<lines; j++)
		{
			auto& line = way->m_lines[j];
			if(line.m_valid)
			{
				stream->Printf("%10d  %02x:%02x:%02x:%02x:%02x:%02x   %d.%d.%d.%d\n",
					line.m_lifetime,
					line.m_mac[0], line.m_mac[1], line.m_mac[2], line.m_mac[3], line.m_mac[4], line.m_mac[5],
					line.m_ip.m_octets[0], line.m_ip.m_octets[1], line.m_ip.m_octets[2], line.m_ip.m_octets[3]
				);
			}
		}
	}
}

bool ParseIPAddress(const char* addr, IPv4Address& ip)
{
	int len = strlen(addr);

	int nfield = 0;
	unsigned int fields[4] = {0};

	//Parse
	for(int i=0; i<len; i++)
	{
		//Dot = move to next field
		if( (addr[i] == '.') && (nfield < 3) )
			nfield ++;

		//Digit = update current field
		else if(isdigit(addr[i]))
			fields[nfield] = (fields[nfield] * 10) + (addr[i] - '0');

		else
			return false;
	}

	//Validate
	if(nfield != 3)
		return false;
	for(int i=0; i<4; i++)
	{
		if(fields[i] > 255)
			return false;
	}

	//Set the IP
	for(int i=0; i<4; i++)
		ip.m_octets[i] = fields[i];
	return true;
}

bool ParseIPAddressWithSubnet(const char* addr, IPv4Address& ip, uint32_t& mask)
{
	int len = strlen(addr);

	int nfield = 0;	//0-3 = IP, 4 = netmask
	unsigned int fields[5] = {0};

	//Parse
	for(int i=0; i<len; i++)
	{
		//Dot = move to next field
		if( (addr[i] == '.') && (nfield < 3) )
			nfield ++;

		//Slash = move to netmask
		else if( (addr[i] == '/') && (nfield == 3) )
			nfield ++;

		//Digit = update current field
		else if(isdigit(addr[i]))
			fields[nfield] = (fields[nfield] * 10) + (addr[i] - '0');

		else
			return false;
	}

	//Validate
	if(nfield != 4)
		return false;
	for(int i=0; i<4; i++)
	{
		if(fields[i] > 255)
			return false;
	}
	if( (fields[4] > 32) || (fields[4] == 0) )
		return false;

	//Set the IP
	for(int i=0; i<4; i++)
		ip.m_octets[i] = fields[i];

	mask = 0xffffffff << (32 - fields[4]);
	return true;
}

void SetIPAddress(CLIOutputStream* stream, const char* addr)
{
	//Parse the base IP address
	uint32_t mask = 0;
	if(!ParseIPAddressWithSubnet(addr, g_ipConfig.m_address, mask))
	{
		stream->Printf("Usage: ip address x.x.x.x/yy\n");
		return;
	}

	//Calculate the netmask
	g_ipConfig.m_netmask.m_octets[0] = (mask >> 24) & 0xff;
	g_ipConfig.m_netmask.m_octets[1] = (mask >> 16) & 0xff;
	g_ipConfig.m_netmask.m_octets[2] = (mask >> 8) & 0xff;
	g_ipConfig.m_netmask.m_octets[3] = (mask >> 0) & 0xff;

	//Calculate the broadcast address
	for(int i=0; i<4; i++)
		g_ipConfig.m_broadcast.m_octets[i] = g_ipConfig.m_address.m_octets[i] | ~g_ipConfig.m_netmask.m_octets[i];
}

void PrintIPAddress(CLIOutputStream* stream)
{
	stream->Printf("IPv4 address: %d.%d.%d.%d\n",
		g_ipConfig.m_address.m_octets[0],
		g_ipConfig.m_address.m_octets[1],
		g_ipConfig.m_address.m_octets[2],
		g_ipConfig.m_address.m_octets[3]
	);

	stream->Printf("Subnet mask:  %d.%d.%d.%d\n",
		g_ipConfig.m_netmask.m_octets[0],
		g_ipConfig.m_netmask.m_octets[1],
		g_ipConfig.m_netmask.m_octets[2],
		g_ipConfig.m_netmask.m_octets[3]
	);

	stream->Printf("Broadcast:    %d.%d.%d.%d\n",
		g_ipConfig.m_broadcast.m_octets[0],
		g_ipConfig.m_broadcast.m_octets[1],
		g_ipConfig.m_broadcast.m_octets[2],
		g_ipConfig.m_broadcast.m_octets[3]
	);
}

void PrintDefaultRoute(CLIOutputStream* stream)
{
	stream->Printf("IPv4 routing table\n");
	stream->Printf("Destination     Gateway\n");
	stream->Printf("0.0.0.0         %d.%d.%d.%d\n",
		g_ipConfig.m_gateway.m_octets[0],
		g_ipConfig.m_gateway.m_octets[1],
		g_ipConfig.m_gateway.m_octets[2],
		g_ipConfig.m_gateway.m_octets[3]);
}

void PrintNTP(CLIOutputStream* stream, STM32NTPClient& ntp)
{
	if(ntp.IsEnabled())
	{
		stream->Printf("NTP client enabled\n");
		auto ip = ntp.GetServerAddress();

		if(ntp.IsSynchronized())
		{
			tm synctime;
			uint16_t syncsub;
			ntp.GetLastSync(synctime, syncsub);

			stream->Printf("Last synchronized to server %d.%d.%d.%d at %04d-%02d-%02dT%02d:%02d:%02d.%04d\n",
				ip.m_octets[0], ip.m_octets[1], ip.m_octets[2], ip.m_octets[3],
				synctime.tm_year + 1900,
				synctime.tm_mon+1,
				synctime.tm_mday,
				synctime.tm_hour,
				synctime.tm_min,
				synctime.tm_sec,
				syncsub);
		}
		else
		{
			stream->Printf("Using server %d.%d.%d.%d (not currently synchronized)\n",
				ip.m_octets[0], ip.m_octets[1], ip.m_octets[2], ip.m_octets[3] );
		}

	}
	else
		stream->Printf("NTP client disabled\n");
}

void PrintSSHKeys(CLIOutputStream* stream, SSHKeyManager& mgr)
{
	stream->Printf("Authorized keys:\n");
	stream->Printf("Slot  Nickname                        Fingerprint\n");

	AcceleratedCryptoEngine tmp;
	char fingerprint[64];

	for(int i=0; i<MAX_SSH_KEYS; i++)
	{
		if(mgr.m_authorizedKeys[i].m_nickname[0] != '\0')
		{
			tmp.GetKeyFingerprint(fingerprint, sizeof(fingerprint), mgr.m_authorizedKeys[i].m_pubkey);
			stream->Printf("%2d    %-30s  SHA256:%s\n",
				i,
				mgr.m_authorizedKeys[i].m_nickname,
				fingerprint);
		}
	}
}
