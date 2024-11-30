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

/**
	@file
	@brief Declaration of SSHKeyManager
 */
#ifndef SSHKeyManager_h
#define SSHKeyManager_h

#ifndef MAX_SSH_KEYS
#define MAX_SSH_KEYS 32
#endif

#include <fpga/AcceleratedCryptoEngine.h>

/**
	@brief A single entry in our authorized_keys list
 */
class AuthorizedKey
{
public:
	uint8_t m_pubkey[ECDSA_KEY_SIZE];
	char m_nickname[MAX_TOKEN_LEN];

	//needed by KVS
	bool operator!= (const AuthorizedKey& rhs) const
	{
		if(memcmp(this, &rhs, sizeof(AuthorizedKey)) != 0)
			return true;
		return false;
	}
};

/**
	@brief Helper for managing a list of SSH keys
 */
class SSHKeyManager
{
public:
	SSHKeyManager();

	void LoadFromKVS(bool log = true);
	void CommitToKVS();

	bool AddPublicKey(const char* keyType, const char* keyBlobBase64, const char* keyDesc);
	void RemovePublicKey(int slot);

	int FindKey(const uint8_t* search);

	///@brief
	AuthorizedKey m_authorizedKeys[MAX_SSH_KEYS];
};

#endif
