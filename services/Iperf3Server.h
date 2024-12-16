/***********************************************************************************************************************
*                                                                                                                      *
* common-embedded-platform                                                                                             *
*                                                                                                                      *
* Copyright (c) 2024 Andrew D. Zonenberg and contributors                                                              *
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
	@brief Embedded network benchmark compatible with a subset of the iperf version 3 protocol

	For now, only supports TCP reverse connection mode for outbound bandwidth benchmarks on the embedded DUT,
	with no bandwidth limit.

	Clientside test command: iperf3 -c $ip -u -R -l 1024
 */
#ifndef Iperf3Server_h
#define Iperf3Server_h

#include <staticnet/net/tcp/TCPServer.h>
#include <staticnet/util/CircularFIFO.h>

#define IPERF_COOKIE_SIZE 37

#ifndef MAX_IPERF_CLIENTS
#define MAX_IPERF_CLIENTS 1
#endif

#define IPERF3_PORT	5201

class IperfConnectionState
{
public:
	IperfConnectionState()
	{ Clear(); }

	/**
		@brief Clears connection state
	 */
	void Clear()
	{
		m_valid = false;
		m_socket = nullptr;
		m_state = IPERF_START;
		memset(m_cookie, 0, sizeof(m_cookie));
		m_mode = MODE_TCP;
		m_time = 0;
		m_bandwidth = 0;
		m_len = 0;
		m_reverseMode = false;
		m_clientPort = 0;
		m_sequence = 0;
		m_rxBuffer.Reset();
	}

	///@brief Position in the connection state machine
	enum
	{
		IPERF_START			= 15,
		PARAM_EXCHANGE		= 9,
		CREATE_STREAMS		= 10,
		TEST_START			= 1,
		TEST_RUNNING		= 2,
		TEST_END			= 4,
		EXCHANGE_RESULTS	= 13,
		DISPLAY_RESULTS		= 14,
		IPERF_DONE			= 16
	} m_state;

	///@brief True if the connection is valid
	bool	m_valid;

	///@brief Socket state handle
	TCPTableEntry* m_socket;

	///@brief The magic cookie chosen by the client for our session (one extra byte so we can null term for printing)
	uint8_t m_cookie[IPERF_COOKIE_SIZE + 1];

	///@brief Packet reassembly buffer (only used for control channel, doesn't have to be big)
	CircularFIFO<256> m_rxBuffer;

	///Operating mode
	enum
	{
		MODE_TCP,
		MODE_UDP
	} m_mode;

	uint32_t m_time;
	uint32_t m_bandwidth;
	uint32_t m_len;
	bool m_reverseMode;
	uint16_t m_clientPort;
	uint32_t m_sequence;
};

class Iperf3Server
	: public TCPServer<MAX_IPERF_CLIENTS, IperfConnectionState>
	, public Task
{
public:
	Iperf3Server(TCPProtocol& tcp, UDPProtocol& udp);

	//Event handlers
	virtual void OnConnectionAccepted(TCPTableEntry* socket) override;
	virtual void OnConnectionClosed(TCPTableEntry* socket) override;
	virtual bool OnRxData(TCPTableEntry* socket, uint8_t* payload, uint16_t payloadLen) override;
	virtual void GracefulDisconnect(int id, TCPTableEntry* socket) override;
	virtual void DropConnection(int id, TCPTableEntry* socket);

	virtual void Iteration() override;

	virtual void OnRxUdpData(IPv4Address srcip, uint16_t sport, uint16_t dport, uint8_t* payload, uint16_t payloadLen);

protected:
	void OnJsonConfigField(int id, const char* name, const char* value);
	void SendState(int id, TCPTableEntry* socket);

	void SendDataOnStream(int id, TCPTableEntry* socket);

	bool OnRxCookie(int id, TCPTableEntry* socket);
	bool OnRxParamExchange(int id, TCPTableEntry* socket);
	bool OnRxEnd(int id, TCPTableEntry* socket);
	bool OnRxDone(int id, TCPTableEntry* socket);

	void FillPacket(int id, uint32_t* payload, uint32_t len);

	//also need a connection to the UDP server
	UDPProtocol& m_udp;
};

#endif
