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
#include "FPGAFirmwareUpdater.h"

///@brief SPI flash controller for FPGA (must be initialized by application code)
APB_SpiFlashInterface* g_fpgaFlash = nullptr;

const uint8_t g_syncword[4] = { 0xaa, 0x99, 0x55, 0x66 };

const char* g_fpgaRegNames[32] =
{
	"CRC",
	"FAR",
	"FDRI",
	"FDRO",
	"CMD",
	"CTL0",
	"MASK",
	"STAT",
	"LOUT",
	"COR0",
	"MFWR",
	"CBC",
	"IDCODE",
	"AXSS",
	"COR1",
	"RSVD_0f",
	"WBSTAR",
	"TIMER",
	"RSVD_12",
	"RSVD_13",
	"RSVD_14",
	"RSVD_15",
	"BOOTSTS",
	"RSVD_17",
	"CTL1",
	"RSVD_19",
	"RSVD_1a",
	"RSVD_1b",
	"RSVD_1c",
	"RSVD_1d",
	"RSVD_1e",
	"BSPI"
};

enum regids
{
	REG_CMD = 0x04
};

enum cmds
{
	CMD_DESYNC = 0x0d
};

const char* g_cmdNames[32] =
{
	"NULL",
	"WCFG",
	"MFW",
	"DHIGH/LFRM",
	"RCFG",
	"START",
	"RCAP",
	"RCRC",
	"AGHIGH",
	"SWITCH",
	"GRESTORE",
	"SHUTDOWN",
	"GCAPTURE",
	"DESYNC",
	"RSVD_0e",
	"IPROG",
	"CRCC",
	"LTIMER",
	"BSPI_READ",
	"FALL_EDGE",
	"RSVD_14",
	"RSVD_15",
	"RSVD_16",
	"RSVD_17",
	"RSVD_18",
	"RSVD_19",
	"RSVD_1a",
	"RSVD_1b",
	"RSVD_1c",
	"RSVD_1d",
	"RSVD_1e",
	"RSVD_1f"
};

FPGAFirmwareUpdater::FPGAFirmwareUpdater(const char* fpgaDevice, uint32_t imageOffset, uint32_t imageSize)
	: m_state(STATE_READ_BITHDR)
	, m_bigWordLen(0)
	, m_deviceName(fpgaDevice)
	, m_imageOffset(imageOffset)
	, m_imageSize(imageSize)
{
}

void FPGAFirmwareUpdater::EraseFlashPartition()
{
	//Erase the image portion of flash
	g_log("Erasing FPGA flash partition...\n");
	LogIndenter li2(g_log);
	auto start = g_logTimer.GetCount();

	//Erase the flash partition
	uint32_t secsize = g_fpgaFlash->GetEraseBlockSize();
	uint32_t nblocks = ( m_imageSize | (secsize - 1) ) / secsize;
	for(uint32_t i=0; i<nblocks; i++)
	{
		uint32_t ptr = m_imageOffset + i*secsize;
		if( (ptr & 0xfffff) == 0)
			g_log("Block %d of %d (%08x)...\n", i, nblocks, reinterpret_cast<uint32_t>(ptr));

		if(!g_fpgaFlash->EraseSector(ptr))
		{
			g_log(Logger::ERROR, "Erase failed\n");
			m_state = STATE_FAILED;
			return;
		}
	}

	//Verify it's blank
	g_log("Blank check...\n");
	const uint32_t rdblocksize = 256;
	uint8_t buf[rdblocksize];
	uint8_t expected[rdblocksize];
	memset(expected, 0xff, rdblocksize);
	for(uint32_t addr=0; addr<m_imageSize; addr += rdblocksize)
	{
		LogIndenter li3(g_log);

		if( (addr & 0xfffff) == 0)
			g_log("%08x...\n", addr);

		g_fpgaFlash->ReadData(addr, buf, rdblocksize);
		if(0 != memcmp(expected, buf, rdblocksize))
		{
			g_log(Logger::ERROR, "Blank check failed\n");

			//Not blank, walk byte by byte and print out the first failing byte
			for(uint32_t i=0; i<rdblocksize; i++)
			{
				if(buf[i] != 0xff)
				{
					g_log(Logger::ERROR, "At 0x%08x: read 0x%02x, expected 0xff\n", addr + i, buf[i]);
					break;
				}
			}

			m_state = STATE_FAILED;
			return;
		}
	}

	auto delta = g_logTimer.GetCount() - start;
	g_log("Flash erase complete (in %d.%d ms)\n", delta / 10, delta % 10);
}

void FPGAFirmwareUpdater::OnDeviceOpened()
{
	m_rxBuffer.Reset();
	m_writeBuffer.Reset();
	m_state = STATE_READ_BITHDR;
	m_bigWordLen = 0;
	m_wptr = m_imageOffset;
}

void FPGAFirmwareUpdater::OnRxData(const uint8_t* data, uint32_t len)
{
	//Push the incoming data into our buffer
	if(!m_rxBuffer.Push(data, len))
	{
		g_log(Logger::ERROR, "RX buffer overflow\n");
		return;
	}

	//Process data until we run out of things to do with the current buffer contents
	while(ProcessDataFromBuffer())
		PushWriteData();
	PushWriteData();
}

void FPGAFirmwareUpdater::OnDeviceClosed()
{
	g_log("Done, flushing remaining data\n");
	FlushWriteData();

	//TODO: reset the FPGA?
}

void FPGAFirmwareUpdater::FlushWriteData()
{
	if(m_state == STATE_FAILED)
		return;

	g_log("Flush: 0x%08x\n", m_wptr);

	//Write any remaining full blocks
	PushWriteData();

	//Write the data
	uint8_t rdbuf[512] = {0};
	auto wbuf = m_writeBuffer.Rewind();
	auto wblock = m_writeBuffer.ReadSize();
	if(!g_fpgaFlash->WriteData(m_wptr, wbuf, wblock))
	{
		g_log(Logger::ERROR, "Flash write of %d bytes failed (at %08x)\n", wblock, m_wptr);
		m_state = STATE_FAILED;
		return;
	}

	//Verify it
	g_fpgaFlash->ReadData(m_wptr, rdbuf, wblock);
	if(0 != memcmp(rdbuf, wbuf, wblock))
	{
		g_log(Logger::ERROR, "Readback failed\n");

		//Walk byte by byte and print out the first failing byte
		for(uint32_t i=0; i<wblock; i++)
		{
			if(rdbuf[i] != wbuf[i])
			{
				g_log(Logger::ERROR, "At 0x%08x: read 0x%02x, expected 0x%02x\n",
					m_wptr + i, rdbuf[i], wbuf[i]);
				break;
			}
		}

		m_state = STATE_FAILED;
		return;
	}

	//Done, clear the write data
	m_writeBuffer.Pop(wblock);
}

/**
	@brief Push write data to the flash chip in max-sized chunks
 */
void FPGAFirmwareUpdater::PushWriteData()
{
	if(m_state == STATE_FAILED)
		return;

	//Make sure we have enough space for verification
	auto wblock = g_fpgaFlash->GetMaxWriteBlockSize();
	uint8_t rdbuf[512] = {0};
	if(wblock > sizeof(rdbuf))
	{
		g_log(Logger::ERROR, "readback buffer too small\n");
		m_state = STATE_FAILED;
		return;
	}

	while(m_writeBuffer.ReadSize() >= wblock)
	{
		//Print progress
		if( (m_wptr & 0xfffff) == 0)
			g_log("Write: 0x%08x\n", m_wptr);

		//Write the data
		auto wbuf = m_writeBuffer.Rewind();
		if(!g_fpgaFlash->WriteData(m_wptr, wbuf, wblock))
		{
			g_log(Logger::ERROR, "Flash write of %d bytes failed (at %08x)\n", wblock, m_wptr);
			m_state = STATE_FAILED;
			return;
		}

		//Verify it
		g_fpgaFlash->ReadData(m_wptr, rdbuf, wblock);

		if(0 != memcmp(rdbuf, wbuf, wblock))
		{
			g_log(Logger::ERROR, "Readback failed\n");

			//Walk byte by byte and print out the first failing byte
			for(uint32_t i=0; i<wblock; i++)
			{
				if(rdbuf[i] != wbuf[i])
				{
					g_log(Logger::ERROR, "At 0x%08x: read 0x%02x, expected 0x%02x\n",
						m_wptr + i, rdbuf[i], wbuf[i]);
					break;
				}
			}

			m_state = STATE_FAILED;
			return;
		}

		//Done, move on
		m_writeBuffer.Pop(wblock);
		m_wptr += wblock;
	}
}

/**
	@brief Process incoming data and parse it
 */
bool FPGAFirmwareUpdater::ProcessDataFromBuffer()
{
	//Turn on to get more extensive debug info about bitstream structure
	const bool verbosePrint = false;

	const unsigned char magic[13] =
	{
		0x00, 0x09, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x00, 0x00, 0x01
	};

	char tmp[257] = {0};

	switch(m_state)
	{
		//Read magic number
		case STATE_READ_BITHDR:

			//Discard 13 byte magic number (always 00 09 0f f0 0f f0 0f f0 0f f0 00 00 01)
			if(m_rxBuffer.ReadSize() >= 13)
			{
				if(0 != memcmp(m_rxBuffer.Rewind(), magic, sizeof(magic)))
				{
					g_log(Logger::ERROR, "Bit file magic number is bad\n");
					m_state = STATE_FAILED;
				}
				else
				{
					m_rxBuffer.Pop(13);
					m_state = STATE_READ_BITHDR_RECORDS;
				}

				return true;
			}
			break;	//STATE_READ_BITHDR

		//Read header records
		case STATE_READ_BITHDR_RECORDS:

			//1 byte record type (always lowercase ascii)
			//1 byte 0x00 (high length byte?)
			//1 byte record length
			//then payload data

			if(m_rxBuffer.ReadSize() >= 3)
			{
				//We have space for the record header, read the length and see if we have the body yet
				auto p = m_rxBuffer.Rewind();
				uint8_t type = p[0];
				uint8_t len = p[2];

				//If this is a 'e' block it's the start of the bitstream
				//Not sure what the length field is for, it doesn't cover the whole bitstream
				//Pop the header and two mystery bytes before the .bin image starts
				if(type == 'e')
				{
					m_rxBuffer.Pop(5);
					m_state = STATE_SYNC_WAIT;
					EraseFlashPartition();
					return true;
				}

				//We have space for the entire payload
				uint32_t blocksize = 3 + len;
				if(m_rxBuffer.ReadSize() >= blocksize )
				{
					//Extract the data until we hit a null or the end of the record
					for(uint8_t i=0; i<len; i++)
					{
						tmp[i] = p[i+3];
						if(tmp[i] == '\0')
							break;
					}

					//Print it
					switch(type)
					{
						case 'a':
							g_log("Bitstream description: %s\n", tmp);
							break;

						case 'b':
							g_log("Target device:         %s\n", tmp);
							if(0 != strcmp(tmp, m_deviceName))
							{
								g_log(Logger::ERROR, "Bitstream was meant for a different FPGA (got %s, expected %s)!\n",
									tmp, m_deviceName);
								m_state = STATE_FAILED;
								break;
							}
							break;

						case 'c':
							g_log("Build date:            %s\n", tmp);
							break;

						case 'd':
							g_log("Build time:            %s\n", tmp);
							break;

						default:
							g_log("Found unknown chunk '%c'\n", type);
							break;
					}

					//Pop the buffer
					m_rxBuffer.Pop(blocksize);
					return true;
				}
			}

			break;	//STATE_READ_BITHDR_RECORDS

		//0xFF padding and bus width detection words before the actual bitstream
		//Needs to be written to flash
		case STATE_SYNC_WAIT:

			//Read as much data as we have, then write to the buffer
			//Look for sync word
			//This is hugely inefficient! Luckily flash writes are so slow it doesn't matter too much...
			//Maybe at some point we can optimize. Not today.
			while(m_rxBuffer.ReadSize() > 4)
			{
				auto ptr = m_rxBuffer.Rewind();

				//If we found the sync word, we're done with the padding/bus width
				if(0 == memcmp(ptr, g_syncword, sizeof(g_syncword)))
				{
					if(verbosePrint)
						g_log("Found sync word\n");
					m_writeBuffer.Push(ptr, sizeof(g_syncword));
					m_rxBuffer.Pop(sizeof(g_syncword));
					m_state = STATE_BITSTREAM;
					return true;
				}

				//No, just write the pre-sync data as is
				m_writeBuffer.Push(*ptr);
				m_rxBuffer.Pop();
			}

			break;

		//Reading actual bitstream data blocks
		case STATE_BITSTREAM:

			if(m_rxBuffer.ReadSize() >= 4)
			{
				//Bitstream consists of config data packets: either type 1 or 2
				//All multi-byte fields are big endian.
				auto ptr = m_rxBuffer.Rewind();
				auto type = ptr[0] >> 5;
				auto pheader = reinterpret_cast<uint32_t*>(ptr);
				uint32_t header = __builtin_bswap32(*pheader);

				//Type 1 packet
				if(type == 1)
				{
					//Read opcode
					auto op = (header >> 27) & 3;
					static const char* ops[4] =
					{
						"nop",
						"read",
						"write",
						"reserved"
					};

					//Read register address
					uint16_t regaddr = (header >> 13) & 0xff;

					//Read word count
					uint32_t len = header & 0x3ff;

					//Make sure we have enough data buffered for the entire packet
					auto packsize = 4 * (len+1);
					if(m_rxBuffer.ReadSize() < packsize )
						return false;

					//Print data
					if(op == 0)
					{
						//Don't print nops to avoid spamming the console log
						//g_log("Nop\n");
					}
					else if( (len == 1) && (regaddr == REG_CMD))
					{
						auto cmd = __builtin_bswap32(pheader[1]) & 0x1f;
						if(verbosePrint)
							g_log("Command: %02x (%s)\n", cmd, g_cmdNames[cmd]);

						//we're finished after this word
						if(cmd == CMD_DESYNC)
							m_state = STATE_DONE;
					}
					else if(len == 1)
					{
						if(verbosePrint)
						{
							g_log("Type 1 %s to %04x (%7s): %08x\n",
								ops[op], regaddr, g_fpgaRegNames[regaddr], __builtin_bswap32(pheader[1]));
						}
					}
					else
					{
						if(verbosePrint)
						{
							g_log("Type 1 %s to %04x (%7s): %d words\n", ops[op], regaddr, g_fpgaRegNames[regaddr], len);
							LogIndenter li(g_log);
							for(uint32_t i=0; i<len; i++)
								g_log("%08x\n", __builtin_bswap32(pheader[i+1]));
						}
					}

					//Buffer it
					m_writeBuffer.Push(ptr, packsize);
					m_rxBuffer.Pop(packsize);
					return true;
				}

				//Type 2 packet
				else if(type == 2)
				{
					//Get the word length
					m_bigWordLen =  header & 0x07ffffff;
					//g_log("found type 2 packet: %u words\n", m_bigWordLen);
					m_state = STATE_BIG_WRITE;

					//Push the header to the write buffer
					m_writeBuffer.Push(ptr, 4);
					m_rxBuffer.Pop(4);
					return true;
				}

				else
				{
					g_log(Logger::ERROR, "Invalid bitstream packet type %d\n", type);
					m_state = STATE_FAILED;
					return true;
				}
			}
			break;

		//Big write (normally to FDRI)
		case STATE_BIG_WRITE:

			if(m_rxBuffer.ReadSize() >= 4)
			{
				//See how many words we have in the buffer
				uint32_t wordcount = m_rxBuffer.ReadSize() / 4;

				//Figure out how many we actually want to process
				//(don't run off the end of the bitstream and start reading finish-up commands)
				if(wordcount >= m_bigWordLen)
					wordcount = m_bigWordLen;

				//Push to write buffer
				auto ptr = m_rxBuffer.Rewind();
				if(!m_writeBuffer.Push(ptr, 4*wordcount))
				{
					g_log(Logger::ERROR, "write buffer overflow\n");
					m_state = STATE_FAILED;
				}
				m_rxBuffer.Pop(4*wordcount);
				m_bigWordLen -= wordcount;

				//If we're done, stop
				if(m_bigWordLen == 0)
				{
					m_state = STATE_BITSTREAM;
					return true;
				}
			}
			return false;

		//Clear buffer if we're done processing stuff
		case STATE_DONE:
		case STATE_FAILED:
			m_rxBuffer.Reset();
			return false;

		default:
			break;
	}

	return false;
}
