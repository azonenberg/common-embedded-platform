/***********************************************************************************************************************
*                                                                                                                      *
* trigger-crossbar                                                                                                     *
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
#include "KeyManagerPubkeyAuthenticator.h"
#include <staticnet/ssh/SSHTransportServer.h>

bool KeyManagerPubkeyAuthenticator::CanUseKey(
	const char* username,
	uint16_t username_len,
	const SSHCurve25519KeyBlob* keyblob,
	bool actualLoginAttempt
	)
{
	if(!SSHTransportServer::StringMatchWithLength(m_username, username, username_len))
		return false;

	//Null terminate username for debug logging
	char nuname[SSH_MAX_USERNAME+1] = {0};
	memcpy(nuname, username, username_len);

	//Check if key is authorized
	auto idx = m_mgr.FindKey(keyblob->m_pubKey);
	if(idx < 0)
	{
		if(actualLoginAttempt)
			g_log("SSH login rejected from user %s using unrecognized key\n", nuname);

		return false;
	}

	//It's good, log it if they're trying to log in (don't log soft queries of "is this key acceptable")
	if(actualLoginAttempt)
		g_log("SSH login attempt from user %s using key %s\n", nuname, m_mgr.m_authorizedKeys[idx].m_nickname);
	return true;
}
