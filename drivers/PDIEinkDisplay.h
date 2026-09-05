/***********************************************************************************************************************
*                                                                                                                      *
* common-embedded-platform                                                                                             *
*                                                                                                                      *
* Copyright (c) 2023-2026 Andrew D. Zonenberg and contributors                                                         *
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

#ifndef PDIEinkDisplay_h
#define PDIEinkDisplay_h

typedef SPI<256, 32> DisplaySPIType;

/**
	@brief PDIEinkDisplay controller

	(this is a task so it can do self-timed refresh operations)
 */
class PDIEinkDisplay : public Task
{
public:
	PDIEinkDisplay(DisplaySPIType* spi, GPIOPin* busy_n, GPIOPin* cs_n, GPIOPin* dc, GPIOPin* rst);

	void StartRefresh(bool forceFullScreenUpdate);
	virtual void Iteration();

	bool IsRefreshInProgress()
	{ return (m_refreshState != STATE_IDLE); }

	void Clear();

	void SetPixel(uint8_t x, uint8_t y, bool black);

	void Text8x16(int16_t x, int16_t y, const char* str, bool black);
	void Text6x8(int16_t x, int16_t y, const char* str, bool black);
	void Line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool black);
	void FilledRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool black);

	void Shutdown()
	{ SendCommand(0x02); }

	//derived class must override this with board specific hook
	virtual uint8_t GetBoardTempC() =0;

protected:
	void LineLow(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool black);
	void LineHigh(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool black);

protected:
	DisplaySPIType* m_spi;
	GPIOPin* m_busy_n;
	GPIOPin* m_cs_n;
	GPIOPin* m_dc;
	GPIOPin* m_rst_n;

	void SendCommand(uint8_t cmd);
	void SendData(uint8_t data);
	uint8_t ReadData();

	//Framebuffer active bitplane
	uint8_t m_framebuffer[2756];

	//Framebuffer old bitplane (used for partial updates)
	uint8_t m_oldFramebuffer[2756];

	const uint16_t m_width;
	const uint16_t m_height;

	enum
	{
		//Idle, awaiting a normal refresh
		STATE_IDLE,

		//Slow refresh path
		STATE_REFRESH_SLOW_INIT,
		STATE_REFRESH_SLOW_DATA0,
		STATE_REFRESH_SLOW_DATA1,

		//Fast refresh path
		STATE_REFRESH_FAST_INIT,
		STATE_REFRESH_FAST_INIT2,
		STATE_REFRESH_FAST_DATA0,
		STATE_REFRESH_FAST_DATA1,

		//Finish a refresh
		STATE_REFRESH_FINAL1,
		STATE_REFRESH_FINAL2,
		STATE_REFRESH_FINAL3,
		STATE_REFRESH_FINAL4,

		//DEBUG: do nothing
		STATE_HANG

	} m_refreshState;

	uint8_t m_psr0;
	uint8_t m_psr1;
};

#endif
