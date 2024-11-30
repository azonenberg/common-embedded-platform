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
#include "SSHKeyManager.h"
#include <staticnet/contrib/base64.h>
#include <staticnet/ssh/SSHCurve25519KeyBlob.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization / serialization

SSHKeyManager::SSHKeyManager()
{

}

void SSHKeyManager::LoadFromKVS(bool log)
{
	if(log)
		g_log("Loading authorized SSH keys\n");
	LogIndenter li(g_log);

	//Clear out our in-memory key database
	memset(m_authorizedKeys, 0, sizeof(m_authorizedKeys));

	//Load database entries
	for(int i=0; i<MAX_SSH_KEYS; i++)
	{
		char keyname[KVS_NAMELEN+1] = {0};
		StringBuffer buf(keyname, KVS_NAMELEN);
		buf.Printf("ssh.authkey%02d", i);

		auto entry = g_kvs->FindObject(keyname);
		if(!entry)
			continue;
		auto ptr = g_kvs->MapObject(entry);

		memcpy(&m_authorizedKeys[i], ptr, sizeof(AuthorizedKey));

		AcceleratedCryptoEngine tmp;
		char fingerprint[64];
		if(m_authorizedKeys[i].m_nickname[0] != '\0')
		{
			tmp.GetKeyFingerprint(fingerprint, sizeof(fingerprint), m_authorizedKeys[i].m_pubkey);
			if(log)
			{
				g_log("%2d    %-30s  SHA256:%s\n",
					i,
					m_authorizedKeys[i].m_nickname,
					fingerprint);
			}
		}
	}
}

void SSHKeyManager::CommitToKVS()
{
	AuthorizedKey empty;
	memset(&empty, 0, sizeof(AuthorizedKey));

	//Save the keys
	for(int i=0; i<MAX_SSH_KEYS; i++)
	{
		char keyname[KVS_NAMELEN+1] = {0};
		StringBuffer buf(keyname, KVS_NAMELEN);
		buf.Printf("ssh.authkey%02d", i);

		//If the nickname is blank, clear the entry fully
		bool blank = false;
		if(m_authorizedKeys[i].m_nickname[0] == 0)
		{
			blank = true;
			memset(&m_authorizedKeys[i], 0, sizeof(AuthorizedKey));
		}

		//Find the existing version of this object (if any)
		auto entry = g_kvs->FindObject(keyname);

		//If no previous entry, and the current entry is blank, no action needed
		if(blank && !entry)
			continue;

		//If current entry is blank, but old one is not, we have to overwrite it
		//Microkvs doesn't currently support deletion of entries!
		//(So if a slot has ever been used in the past, we have to write a blank value there if we're not using it)
		//Obviously if we have valid data, we need to write it too

		//Save the key blob
		g_kvs->StoreObjectIfNecessary(keyname, m_authorizedKeys[i], empty);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Key creation / removal

/**
	@brief Adds a new public key to the database of authorized users

	@param keyType			Constant string "ssh-ed25519", will fail if anything else is passed
	@param keyBlobBase64	Base64 encoded public key blob
	@param keyDesc			Human-readable key description, e.g. foo@bar

	@return	True if successfully added (or already present), false if an error occurred
 */
bool SSHKeyManager::AddPublicKey(
	const char* keyType,
	const char* keyBlobBase64,
	const char* keyDesc
	)
{
	g_log("Adding SSH public key with type=%s, blob=%s, desc=%s\n",
		keyType, keyBlobBase64, keyDesc);

	//If type isn't ssh-ed25519 fail
	if(0 != strcmp(keyType, "ssh-ed25519"))
		return false;

	//Decode the key blob
	char keyblob[64] = {0};
	auto bloblen = strlen(keyBlobBase64);
	if(bloblen > 84)
	{
		g_log(Logger::ERROR, "Public key blob is too long (invalid)\n");
		return false;
	}
	base64_decodestate ctx;
	base64_init_decodestate(&ctx);
	auto binlen = base64_decode_block(keyBlobBase64, bloblen, keyblob, &ctx);

	//Validate the key blob
	if(binlen != 51)
	{
		g_log(Logger::ERROR, "Public key blob decoded to invalid length (expected 51 bytes got %d)\n", binlen);
		return false;
	}
	auto blob = reinterpret_cast<SSHCurve25519KeyBlob*>(keyblob);
	blob->ByteSwap();
	if(blob->m_keyTypeLength != 11)
	{
		g_log(Logger::ERROR, "Public key blob type has invalid length (expected 11 bytes got %d)\n", blob->m_keyTypeLength);
		return false;
	}
	if(memcmp(blob->m_keyType, "ssh-ed25519", 11) != 0)
	{
		g_log(Logger::ERROR, "Public key blob type has invalid type (expected ssh-ed25519)\n");
		return false;
	}
	if(blob->m_pubKeyLength != ECDSA_KEY_SIZE)
	{
		g_log(Logger::ERROR, "Public key blob type has invalid  key length (expected %d bytes got %d)\n", ECDSA_KEY_SIZE, blob->m_pubKeyLength);
		return false;
	}

	//Check if we already have the key internally
	//If not, save to the first free slot in RAM (don't update on flash unless we commit)
	bool foundFree = false;
	int freeSlot = 0;
	for(int i=0; i<MAX_SSH_KEYS; i++)
	{
		//If the slot is unoccupied, make a note of that
		if(!foundFree && m_authorizedKeys[i].m_nickname[0] == '\0')
		{
			foundFree = true;
			freeSlot = i;
			continue;
		}

		//If key matches, update the nickname and stop
		if(0 == memcmp(m_authorizedKeys[i].m_pubkey, blob->m_pubKey, ECDSA_KEY_SIZE))
		{
			strncpy(m_authorizedKeys[i].m_nickname, keyDesc, KVS_NAMELEN);
			m_authorizedKeys[i].m_nickname[KVS_NAMELEN] = 0;
			return true;
		}
	}

	//If we did not have the key, but there's a free slot, save it
	if(foundFree)
	{
		memcpy(m_authorizedKeys[freeSlot].m_pubkey, blob->m_pubKey, ECDSA_KEY_SIZE);
		strncpy(m_authorizedKeys[freeSlot].m_nickname, keyDesc, KVS_NAMELEN);
		m_authorizedKeys[freeSlot].m_nickname[KVS_NAMELEN] = 0;
		return true;
	}

	//No space to save it
	else
	{
		g_log(Logger::ERROR, "Could not add SSH key (all %d slots in use)\n", MAX_SSH_KEYS);
		return false;
	}
}

/**
	@brief Removes a key from the authorized_keys list
 */
void SSHKeyManager::RemovePublicKey(int slot)
{
	if( (slot >= 0) && (slot < MAX_SSH_KEYS) )
		memset(&m_authorizedKeys[slot], 0, sizeof(AuthorizedKey));
}

/**
	@brief Checks if a given key is in the authorized keys list

	@return Zero or greater: slot of the key
			Negative: not found
 */
int SSHKeyManager::FindKey(const uint8_t* search)
{
	for(int i=0; i<MAX_SSH_KEYS; i++)
	{
		if(0 == memcmp(m_authorizedKeys[i].m_pubkey, search, ECDSA_KEY_SIZE))
			return i;
	}
	return -1;
}
