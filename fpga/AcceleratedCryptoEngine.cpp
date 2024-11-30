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
#include "AcceleratedCryptoEngine.h"
#include "../../../staticnet/contrib/tweetnacl_25519.h"

//curve25519 unpacked base point: {X, Y, gf1, X*Y}
//(1 and x*y now computed on FPGA)
//TODO: this is a constant, we should be able to store this on the FPGA somewhere and not waste SPI BW
//pushing it each time we need it
static const uint8_t g_curve25519BasePointUnpacked[64] =
{
	0x1a, 0xd5, 0x25, 0x8f, 0x60, 0x2d, 0x56, 0xc9, 0xb2, 0xa7, 0x25, 0x95, 0x60, 0xc7, 0x2c, 0x69,
	0x5c, 0xdc, 0xd6, 0xfd, 0x31, 0xe2, 0xa4, 0xc0, 0xfe, 0x53, 0x6e, 0xcd, 0xd3, 0x36, 0x69, 0x21,
	0x58, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
	0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66
};

AcceleratedCryptoEngine::AcceleratedCryptoEngine()
{
}

void AcceleratedCryptoEngine::BlockUntilAcceleratorDone()
{
	//StatusRegisterMaskedWait(&g_curve25519->status, &g_curve25519->status2, 0x1, 0x0);
	g_log(Logger::ERROR, "Not implemented\n");
	while(1)
	{}
}

/**
	@brief Debug utility for printing a key to the console
 */
void AcceleratedCryptoEngine::PrintBlock(const char* keyname, const uint8_t* key)
{
	/*
	g_cliUART.Printf("%20s = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
		"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		keyname,
		key[31], key[30], key[29], key[28], key[27], key[26], key[25], key[24],
		key[23], key[22], key[21], key[20], key[19], key[18], key[17], key[16],
		key[15], key[14], key[13], key[12], key[11], key[10], key[9], key[8],
		key[7], key[6], key[5], key[4], key[3], key[2], key[1], key[0]);
	*/
}

void AcceleratedCryptoEngine::SharedSecret(uint8_t* sharedSecret, uint8_t* clientPublicKey)
{
	/*
	#ifdef CRYPTO_PROFILE
	auto t1 = g_logTimer->GetCount();
	#endif

	g_apbfpga.BlockingWriteN(g_curve25519->e, m_ephemeralkeyPriv, ECDH_KEY_SIZE);
	g_apbfpga.BlockingWriteN(g_curve25519->work, clientPublicKey, ECDH_KEY_SIZE);
	g_apbfpga.BlockingWrite32(&g_curve25519->cmd, CMD_CRYPTO_SCALARMULT);
	BlockUntilAcceleratorDone();
	memcpy(sharedSecret, (void*)g_curve25519->data_out, ECDH_KEY_SIZE);

	#ifdef CRYPTO_PROFILE
	auto delta = g_logTimer->GetCount() - t1;
	g_log("AcceleratedCryptoEngine::SharedSecret (FPGA acceleration): %d.%d ms\n", delta/10, delta%10);
	#endif
	*/
	g_log(Logger::ERROR, "Not implemented\n");
	while(1)
	{}
}

/**
	@brief Generates an x25519 key pair.

	The private key is kept internal to the CryptoEngine object.

	The public key is stored in the provided buffer, which must be at least 32 bytes in size.
 */
void AcceleratedCryptoEngine::GenerateX25519KeyPair(uint8_t* pub)
{
	/*
	#ifdef CRYPTO_PROFILE
	auto t1 = g_logTimer->GetCount();
	#endif

	//To be a valid key, a few bits need well-defined values. The rest are cryptographic randomness.
	GenerateRandom(m_ephemeralkeyPriv, 32);
	m_ephemeralkeyPriv[0] &= 0xF8;
	m_ephemeralkeyPriv[31] &= 0x7f;
	m_ephemeralkeyPriv[31] |= 0x40;

	//Well defined curve25519 base point from crypto_scalarmult_base
	static uint8_t basepoint[ECDH_KEY_SIZE] =
	{
		9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	//Make the FPGA do the rest of the work
	g_apbfpga.BlockingWriteN(g_curve25519->e, m_ephemeralkeyPriv, ECDH_KEY_SIZE);
	g_apbfpga.BlockingWriteN(g_curve25519->work, basepoint, ECDH_KEY_SIZE);
	g_apbfpga.BlockingWrite32(&g_curve25519->cmd, CMD_CRYPTO_SCALARMULT);
	BlockUntilAcceleratorDone();
	memcpy(pub, (void*)g_curve25519->data_out, ECDH_KEY_SIZE);

	#ifdef CRYPTO_PROFILE
	auto delta = g_logTimer->GetCount() - t1;
	g_log("AcceleratedCryptoEngine::GenerateX25519KeyPair (FPGA acceleration): %d.%d ms\n", delta/10, delta%10);
	#endif
	*/
	g_log(Logger::ERROR, "Not implemented\n");
	while(1)
	{}
}

/**
	@brief Verify a signed message

	The signature is *prepended* to the message: first 64 bytes are signature, then the message
 */
bool AcceleratedCryptoEngine::VerifySignature(uint8_t* signedMessage, uint32_t lengthIncludingSignature, uint8_t* publicKey)
{
	/*
	#ifdef CRYPTO_PROFILE
	auto t1 = g_logTimer->GetCount();
	#endif

	//fixed length cap
	if(lengthIncludingSignature > 1024)
		return false;

	//If message isn't big enough to even have a signature it can't be valid
	if (lengthIncludingSignature < ECDSA_SIG_SIZE)
		return false;

	//Hash the input message then do a modular reduction to make sure it stays within our field
	uint8_t hash[SHA512_DIGEST_SIZE];
	unsigned char tmpbuf[1024];
	memcpy(tmpbuf, signedMessage, lengthIncludingSignature);
	memcpy(tmpbuf + ECDSA_KEY_SIZE, publicKey, ECDSA_KEY_SIZE);
	crypto_hash(hash, tmpbuf, lengthIncludingSignature);
	reduce(hash);

	//Unpack the packed public key
	gf q[4];
	gf p[4];
	if (unpackneg(q, publicKey))
		return false;

	#ifdef CRYPTO_PROFILE
	auto t2 = g_logTimer->GetCount();
	#endif

	//Expanded public key for sending to accelerator
	//TODO: can we move the expansion to the FPGA to save SPI BW and speed compute?
	uint8_t qref[64];
	pack25519(&qref[0], q[0]);
	pack25519(&qref[32], q[1]);

	//Do the first scalarmult() on the FPGA

	//Calculate the expected signature
	//scalarmult(p, q, hash);
	g_apbfpga.BlockingWriteN(g_curve25519->e, hash, ECDSA_KEY_SIZE);
	g_apbfpga.BlockingWriteN(g_curve25519->q0, qref, ECDSA_KEY_SIZE*2);
	BlockUntilAcceleratorDone();
	uint8_t pfpga[128];
	for(int block=0; block<4; block++)
	{
		g_curve25519->rd_addr = block;
		asm("dmb st");
		memcpy(pfpga + block*32, (void*)g_curve25519->data_out, ECDH_KEY_SIZE);
	}

	#ifdef CRYPTO_PROFILE
	auto t3 = g_logTimer->GetCount();
	#endif

	//scalarbase(q, signedMessage + 32);
	g_apbfpga.BlockingWriteN(g_curve25519->e, signedMessage+32, ECDSA_KEY_SIZE);
	g_apbfpga.BlockingWriteN(g_curve25519->base_q0, g_curve25519BasePointUnpacked, ECDSA_KEY_SIZE);
	BlockUntilAcceleratorDone();
	uint8_t qfpga[128];
	for(int block=0; block<4; block++)
	{
		g_curve25519->rd_addr = block;
		asm("dmb st");
		memcpy(qfpga + block*32, (void*)g_curve25519->data_out, ECDH_KEY_SIZE);
	}

	#ifdef CRYPTO_PROFILE
	auto t4 = g_logTimer->GetCount();
	#endif

	//Unpack results from the FPGA
	unpack25519(p[0], &pfpga[0]);
	unpack25519(p[1], &pfpga[32]);
	unpack25519(p[2], &pfpga[64]);
	unpack25519(p[3], &pfpga[96]);
	unpack25519(q[0], &qfpga[0]);
	unpack25519(q[1], &qfpga[32]);
	unpack25519(q[2], &qfpga[64]);
	unpack25519(q[3], &qfpga[96]);

	//Final addition... we really should try to keep this on the FPGA if possible
	add(p,q);
	uint8_t t[32];

	pack(t,p);

	//Final signature verification
	if (crypto_verify_32(signedMessage, t))
		return false;

	#ifdef CRYPTO_PROFILE
	auto tend = g_logTimer->GetCount();
	auto delta = tend - t1;
	g_log("AcceleratedCryptoEngine::VerifySignature (FPGA acceleration): %d.%d ms\n", delta/10, delta%10);
	LogIndenter li(g_log);
	delta = t2-t1;
	g_log("Setup (no acceleration): %d.%d ms\n", delta/10, delta%10);
	delta = t3-t2;
	g_log("scalarmult (FPGA acceleration): %d.%d ms\n", delta/10, delta%10);
	delta = t4-t3;
	g_log("scalarbase (FPGA acceleration): %d.%d ms\n", delta/10, delta%10);
	delta = tend-t4;
	g_log("Final (no acceleration): %d.%d ms\n", delta/10, delta%10);
	#endif

	return true;*/

	g_log(Logger::ERROR, "Not implemented\n");
	while(1)
	{}
	return false;
}

///@brief Signs an exchange hash with our host key
void AcceleratedCryptoEngine::SignExchangeHash(uint8_t* sigOut, uint8_t* exchangeHash)
{
	/*
	#ifdef CRYPTO_PROFILE
	auto t1 = g_logTimer->GetCount();
	#endif

	//Hash the private key and massage it to make sure it's a valid curve point
	uint8_t privkeyHash[64];
	crypto_hash(privkeyHash, m_hostkeyPriv, 32);
	privkeyHash[0] &= 248;
	privkeyHash[31] &= 127;
	privkeyHash[31] |= 64;

	//Build the actual buffer we're signing
	uint8_t sm[128];
	memcpy(sm+64, exchangeHash, SHA256_DIGEST_SIZE);
	memcpy(sm+32, privkeyHash+32, 32);

	//Hash the buffer and reduce it to make sure it's within our field
	uint8_t bufferHash[64];
	crypto_hash(bufferHash, sm + 32, SHA256_DIGEST_SIZE + 32);
	reduce(bufferHash);

	//Actual signing stuff
	//scalarbase(p,bufferHash);
	g_apbfpga.BlockingWriteN(g_curve25519->e, bufferHash, ECDSA_KEY_SIZE);
	g_apbfpga.BlockingWriteN(g_curve25519->base_q0, g_curve25519BasePointUnpacked, ECDSA_KEY_SIZE);
	BlockUntilAcceleratorDone();
	uint8_t pfpga[128];
	for(int block=0; block<4; block++)
	{
		g_curve25519->rd_addr = block;
		asm("dmb st");
		memcpy(pfpga + block*32, (void*)g_curve25519->data_out, ECDH_KEY_SIZE);
	}

	//Unpack and repack the result and save in q
	//Optimization: skip processing of the final word since it's not used by pack()
	gf p[4];
	unpack25519(p[0], &pfpga[0]);
	unpack25519(p[1], &pfpga[32]);
	unpack25519(p[2], &pfpga[64]);
	//unpack25519(p[3], &pfpga[96]);

	//pack(sm,p);
	gf tx, ty, zi;
	inv25519(zi, p[2]);
	M(tx, p[0], zi);
	M(ty, p[1], zi);
	pack25519(sm, ty);
	sm[31] ^= par25519(tx) << 7;

	//Hash the public key
	uint8_t msgHash[64];
	memcpy(sm+32, m_hostkeyPub, ECDSA_KEY_SIZE);
	crypto_hash(msgHash, sm, SHA256_DIGEST_SIZE + 64);
	reduce(msgHash);

	//Bignum stuff on output
	int64_t x[64] = {0};
	for(int i=0; i<32; i++)
		x[i] = (u64) bufferHash[i];
	for(int i=0; i<32; i++)
	{
		for(int j=0; j<32; j++)
			x[i+j] += msgHash[i] * (u64) privkeyHash[j];
	}

	//Final modular reduction and output
	modL(sm + 32,x);
	memcpy(sigOut, sm, 64);

	#ifdef CRYPTO_PROFILE
	auto delta = g_logTimer->GetCount() - t1;
	g_log("AcceleratedCryptoEngine::SignExchangeHash (FPGA acceleration): %d.%d ms\n", delta/10, delta%10);
	#endif
	*/

	g_log(Logger::ERROR, "Not implemented\n");
	while(1)
	{}
}
