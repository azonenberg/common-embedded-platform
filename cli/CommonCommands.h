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

#ifndef CEP_CLI_CommonCommands_h
#define CEP_CLI_CommonCommands_h

#include <core/platform.h>
#include <staticnet/stack/staticnet.h>
#include <embedded-cli/CLIOutputStream.h>
#include <embedded-cli/CLISessionContext.h>
#include "../tcpip/CommonTCPIP.h"
#include "../tcpip/SSHKeyManager.h"
#include "../services/STM32NTPClient.h"

class EthernetProtocol;

void PrintProcessorInfo(CLIOutputStream* stream);

void PrintFlashSummary(CLIOutputStream* stream);
void PrintFlashDetails(CLIOutputStream* stream, const char* objectName);
void RemoveFlashKey(CLIOutputStream* stream, const char* key);

void PrintSSHHostKey(CLIOutputStream* stream);

void PrintARPCache(CLIOutputStream* stream, EthernetProtocol* eth);

void SetIPAddress(CLIOutputStream* stream, const char* addr);

bool ParseIPAddress(const char* addr, IPv4Address& ip);
bool ParseIPAddressWithSubnet(const char* addr, IPv4Address& ip, uint32_t& mask);

void PrintNTP(CLIOutputStream* stream, STM32NTPClient& ntp);

void PrintSSHKeys(CLIOutputStream* stream, SSHKeyManager& mgr);

void PrintIPAddress(CLIOutputStream* stream);
void PrintDefaultRoute(CLIOutputStream* stream);

#endif
