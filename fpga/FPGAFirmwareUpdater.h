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

#ifndef FPGAFirmwareUpdater_h
#define FPGAFirmwareUpdater_h

#define BIT_RX_BUFFER_SIZE 4096

#include <staticnet/util/CircularFIFO.h>
#include <embedded-utils/APB_SpiFlashInterface.h>

/**
	@brief Helper for SFTP firmware updates of the FPGA
 */
class FPGAFirmwareUpdater
{
public:
	FPGAFirmwareUpdater(
		const char* fpgaDevice,
		uint32_t imageOffset	= 0x0000'0000,
		uint32_t imageSize		= 0x0040'0000
		);

	///@brief Called when the device file is opened
	void OnDeviceOpened();

	///@brief Called when new data arrives
	void OnRxData(const uint8_t* data, uint32_t len);

	///@brief Called when the device file is closed (update complete)
	void OnDeviceClosed();

protected:
	void EraseFlashPartition();

	enum
	{
		STATE_READ_BITHDR,
		STATE_READ_BITHDR_RECORDS,
		STATE_SYNC_WAIT,
		STATE_BITSTREAM,
		STATE_BIG_WRITE,
		STATE_DONE,
		STATE_FAILED
	} m_state;

	bool ProcessDataFromBuffer();
	void PushWriteData();
	void FlushWriteData();

	///@brief Buffer for handling incoming data
	CircularFIFO<BIT_RX_BUFFER_SIZE> m_rxBuffer;

	///@brief Buffer for handling write data
	CircularFIFO<BIT_RX_BUFFER_SIZE> m_writeBuffer;

	uint32_t m_bigWordLen;
	uint32_t m_wptr;

	///@brief Expected device name
	const char* m_deviceName;

	///@brief Offset of the FPGA image within flash
	uint32_t m_imageOffset;

	///@brief Size of the FPGA image
	uint32_t m_imageSize;
};

extern APB_SpiFlashInterface* g_fpgaFlash;

#endif
