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

#ifndef AcceleratedCryptoEngine_h
#define AcceleratedCryptoEngine_h

#include <staticnet/drivers/stm32/STM32CryptoEngine.h>
#include <APB_Curve25519.h>

extern volatile APB_Curve25519 FCURVE25519;

//TODO: this is a hack and should get moved somewhere librarized etc
#ifdef QSPI_CACHE_WORKAROUND
void StatusRegisterMaskedWait(volatile uint32_t* a, volatile uint32_t* b, uint32_t mask, uint32_t target);
void SfrMemcpy(volatile void* dst, void* src, uint32_t len);
#include "../../firmware/main/bsp/FPGAInterface.h"
#include "../../firmware/main/bsp/APBFPGAInterface.h"
extern APBFPGAInterface g_apbfpga;
#endif

/**
	@brief Extension of STM32CryptoEngine using our FPGA curve25519 accelerator
 */
class AcceleratedCryptoEngine : public STM32CryptoEngine
{
public:
	AcceleratedCryptoEngine();

	virtual void GenerateX25519KeyPair(uint8_t* pub) override;
	virtual void SharedSecret(uint8_t* sharedSecret, uint8_t* clientPublicKey) override;
	virtual bool VerifySignature(uint8_t* signedMessage, uint32_t lengthIncludingSignature, uint8_t* publicKey) override;
	virtual void SignExchangeHash(uint8_t* sigOut, uint8_t* exchangeHash) override;

protected:
	void BlockUntilAcceleratorDone()
	{
		asm("dmb st");
		#ifdef QSPI_CACHE_WORKAROUND
			StatusRegisterMaskedWait(&FCURVE25519.status, &FCURVE25519.status2, 0x1, 0x0);
		#else
			while(FCURVE25519.status != 0)
			{}
		#endif
	}

	void PrintBlock(const char* keyname, const uint8_t* key);
};

#endif
