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

#include <core/platform.h>
#include <staticnet-config.h>
#include <staticnet/stack/staticnet.h>
#include "Iperf3Server.h"

//For now assume we're on a STM32 with RTC
#include <stm32.h>
#include <peripheral/RTC.h>

Iperf3Server::Iperf3Server(TCPProtocol& tcp, UDPProtocol& udp)
	: TCPServer(tcp)
	, m_udp(udp)
{
	//Register ourselves automatically in the task table
	g_tasks.push_back(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Event handlers

/**
	@brief Handles a newly accepted connection
 */
void Iperf3Server::OnConnectionAccepted(TCPTableEntry* socket)
{
	//Make a new entry in the socket state table
	int id = AllocateConnectionID(socket);
	if(id < 0)
		return;

	m_state[id].m_state = IperfConnectionState::IPERF_START;
}

/**
	@brief Tears down a connection when the socket is closed
 */
void Iperf3Server::OnConnectionClosed(TCPTableEntry* socket)
{
	//Connection was terminated by the other end, close our state so we can reuse it
	auto id = GetConnectionID(socket);
	if(id >= 0)
		m_state[id].Clear();
}

/**
	@brief Handler for incoming TCP segments
 */
bool Iperf3Server::OnRxData(TCPTableEntry* socket, uint8_t* payload, uint16_t payloadLen)
{
	//Look up the connection ID for the incoming session
	auto id = GetConnectionID(socket);
	if(id < 0)
		return true;

	//Push the segment data into our RX FIFO
	if(!m_state[id].m_rxBuffer.Push(payload, payloadLen))
	{
		DropConnection(id, socket);
		return false;
	}

	//Figure out what state we're in so we know what to expect
	while(true)
	{
		switch(m_state[id].m_state)
		{
			//Starting a connection
			case IperfConnectionState::IPERF_START:
				if(!OnRxCookie(id, socket))
					return true;
				break;

			//Exchanging parameters with the client
			case IperfConnectionState::PARAM_EXCHANGE:
				if(!OnRxParamExchange(id, socket))
					return true;
				break;

			//We're creating streams (nothing to do, wait for client to connectO
			case IperfConnectionState::CREATE_STREAMS:
				return true;

			//Not possible, we never return in this state
			case IperfConnectionState::TEST_START:
				return false;

			//Test is running, stop
			//expect TEST_END from client to stop us, we respond with EXCHANGE_RESULTS
			case IperfConnectionState::TEST_RUNNING:
				if(!OnRxEnd(id, socket))
					return true;
				return true;

			//Disconnect
			case IperfConnectionState::EXCHANGE_RESULTS:
				if(!OnRxDone(id, socket))
					return true;
				return true;

			//unknown state, stop
			default:
				return false;
		}
	}
}

/**
	@brief Connection is done, ACK and discard whatever is sent to us until the client disconnects
 */
bool Iperf3Server::OnRxDone([[maybe_unused]] int id, [[maybe_unused]] TCPTableEntry* socket)
{
	return true;
}

/**
	@brief Handler for incoming UDP packets
 */
void Iperf3Server::OnRxUdpData(
	IPv4Address srcip,
	uint16_t sport,
	uint16_t dport,
	[[maybe_unused]] uint8_t* payload,
	[[maybe_unused]] uint16_t payloadLen)
{
	//Ignore anything not to our pot
	if(dport != IPERF3_PORT)
		return;

	//Check if we have any client from the same IP in CREATE_STREAMS state.
	//If so this is probably a request to open it.
	//(TODO: is there a better way to match?)
	for(size_t i=0; i<MAX_IPERF_CLIENTS; i++)
	{
		if(!m_state[i].m_valid)
			continue;
		if(m_state[i].m_state != IperfConnectionState::CREATE_STREAMS)
			continue;
		if(m_state[i].m_socket->m_remoteIP != srcip)
			continue;

		//Ok, we found it.
		//Client sent us datagram containing "9876" (TODO validate that was in fact the payload)
		//Respond with "6789"
		auto upack = m_udp.GetTxPacket(m_state[i].m_socket->m_remoteIP);
		if(!upack)
			return;
		memcpy(upack->Payload(), "6789", 4);
		m_udp.SendTxPacket(upack, IPERF3_PORT, sport, 4);

		//Update the state to "start"
		g_log("Stream opened (client port %d)\n", sport);
		m_state[i].m_clientPort = sport;
		m_state[i].m_state = IperfConnectionState::TEST_START;
		SendState(i, m_state[i].m_socket);
		break;
	}
}

/**
	@brief Gracefully disconnects from a session
 */
void Iperf3Server::GracefulDisconnect([[maybe_unused]] int id, [[maybe_unused]] TCPTableEntry* socket)
{
	m_state[id].Clear();
	m_tcp.CloseSocket(socket);
}

/**
	@brief Drops a connection due to a protocol error or similar
 */
void Iperf3Server::DropConnection(int id, TCPTableEntry* socket)
{
	m_state[id].Clear();
	m_tcp.CloseSocket(socket);
}

/**
	@brief Sends the current state to the client
 */
void Iperf3Server::SendState(int id, TCPTableEntry* socket)
{
	auto segment = m_tcp.GetTxSegment(socket);
	if(!segment)
		return;
	auto payload = segment->Payload();
	payload[0] = m_state[id].m_state;
	m_tcp.SendTxSegment(socket, segment, 1);
}

/**
	@brief Reads the connection cookie

	@return true if processed, false if not enough data
 */
bool Iperf3Server::OnRxCookie(int id, TCPTableEntry* socket)
{
	//Need 37 bytes in the buffer
	auto& fifo = m_state[id].m_rxBuffer;
	if(fifo.ReadSize() < IPERF_COOKIE_SIZE)
		return false;

	//All good, read the cookie
	auto p = fifo.Rewind();
	memcpy(m_state[id].m_cookie, p, IPERF_COOKIE_SIZE);
	g_log("Iperf3 client connected (cookie=%s)\n", m_state[id].m_cookie);
	fifo.Pop(IPERF_COOKIE_SIZE);

	//Exchanging parameters
	m_state[id].m_state = IperfConnectionState::PARAM_EXCHANGE;
	SendState(id, socket);
	return true;
}

bool Iperf3Server::OnRxEnd(int id, TCPTableEntry* socket)
{
	auto& fifo = m_state[id].m_rxBuffer;
	auto p = fifo.Rewind();

	//end command
	if(p[0] != IperfConnectionState::TEST_END)
	{
		g_log(Logger::WARNING, "Expected TEST_END from client, got something else\n");
		fifo.Pop(1);
		return false;
	}

	//All done, now in EXCHANGE_RESULTS state
	//g_log("Got END command from client\n");
	fifo.Pop(1);

	//Exchange results
	m_state[id].m_state = IperfConnectionState::EXCHANGE_RESULTS;
	SendState(id, socket);

	//Send the results (include some bogus fields since we don't have cpu usage accounting
	auto segment = m_tcp.GetTxSegment(socket);
	if(!segment)
		return false;
	auto payload = segment->Payload();

	StringBuffer buf((char*)payload+4, 1400);
	buf.Printf(
		"{"
		"\"cpu_util_total\":0.0,"
		"\"cpu_util_user\":0.0,"
		"\"cpu_util_system\":0.0,"
		"\"sender_has_retransmits\":0,"
		"\"streams\":[{"
		"\"id\":1,"
		"\"bytes\":%u,"
		"\"retransmits\":18446744073709551615,"	// -1 casted to an unsigned int64, yes this is what iperf expects
		"\"jitter\":0.0,"
		"\"errors\":0.0,"
		"\"packets\":%d,"
		"\"start_time\":0,"
		"\"end_time\":%d.%d"
		"}]}",
		m_state[id].m_sequence * m_state[id].m_len,
		m_state[id].m_sequence,

		//TODO: actual measured elapsed sending time?
		m_state[id].m_time,
		0
	);

	//Prepend length of json blob as big endian uint32
	payload[0] = 0;
	payload[1] = 0;
	payload[2] = buf.length() >> 8;
	payload[3] = buf.length() & 0xff;

	//Ask client to display results
	payload[4+buf.length()] = IperfConnectionState::DISPLAY_RESULTS;
	m_tcp.SendTxSegment(socket, segment, buf.length() + 5);
	return true;
}

/**
	@brief Reads the configuration blob from the client
 */
bool Iperf3Server::OnRxParamExchange(int id, TCPTableEntry* socket)
{
	//Need a minimum of 6 bytes (4 byte length + curly braces)
	auto& fifo = m_state[id].m_rxBuffer;
	if(fifo.ReadSize() < 6)
		return false;

	//Peek the parameter length
	auto p = fifo.Rewind();
	auto len = __builtin_bswap32(*reinterpret_cast<uint32_t*>(p));
	if(fifo.ReadSize() < 4+len)
		return false;

	g_log("Got parameters from client\n");
	LogIndenter li(g_log);

	//Buffer is fully received, parse the JSON

	//Sanity check
	auto json = p+4;
	if(json[0] != '{')
	{
		g_log(Logger::ERROR, "Invalid JSON blob (missing opening curly brace)\n");
		return false;
	}

	//Quick and dirty parser for the subset of stuff we care about ("name":value)
	enum
	{
		STATE_OPEN_QUOTE,
		STATE_NAME,
		STATE_COLON,
		STATE_VALUE
	} jsonState = STATE_OPEN_QUOTE;

	char name[32];
	char value[32];
	uint32_t j = 0;

	for(uint32_t i = 1; i<len; i++)
	{
		switch(jsonState)
		{
			//Expect opening quote
			case STATE_OPEN_QUOTE:
				if(json[i] != '\"')
				{
					g_log(Logger::ERROR, "Invalid JSON blob (expected opening quote)\n");
					DropConnection(id, socket);
					return true;
				}
				jsonState = STATE_NAME;
				j = 0;
				break;

			//Read the name
			case STATE_NAME:
				if(j >= 32)
				{
					g_log(Logger::ERROR, "Invalid JSON blob (name too long)\n");
					DropConnection(id, socket);
					return true;
				}
				else if(json[i] == '\"')
				{
					name[j] = '\0';
					jsonState = STATE_COLON;
				}
				else
					name[j++] = json[i];
				break;

			//Expect colon
			case STATE_COLON:
				if(json[i] != ':')
				{
					g_log(Logger::ERROR, "Invalid JSON blob (expected colon)\n");
					DropConnection(id, socket);
					return true;
				}
				jsonState = STATE_VALUE;
				j = 0;
				break;

			//Read the value
			case STATE_VALUE:
				if(j >= 32)
				{
					g_log(Logger::ERROR, "Invalid JSON blob (value too long)\n");
					DropConnection(id, socket);
					return true;
				}
				else if( (json[i] == ',') || (json[i] == '}') )
				{
					value[j] = '\0';
					OnJsonConfigField(id, name, value);
					jsonState = STATE_OPEN_QUOTE;
				}
				else
					value[j++] = json[i];

				//end of message
				if(json[i] == '}')
					i = len;
				break;
		}
	}

	//Pop the JSON
	fifo.Pop(4+len);

	//Transition to "create streams"
	m_state[id].m_state = IperfConnectionState::CREATE_STREAMS;
	SendState(id, socket);

	//Validate that we're in UDP mode
	if(m_state[id].m_mode != IperfConnectionState::MODE_UDP)
	{
		g_log(Logger::WARNING, "TCP mode not yet supported\n");
		DropConnection(id, socket);
		return true;
	}

	//Validate that we're in reverse mode
	if(m_state[id].m_reverseMode != true)
	{
		g_log(Logger::WARNING, "Only reverse mode supported right now\n");
		DropConnection(id, socket);
		return true;
	}

	if(m_state[id].m_len >= 1480)
	{
		g_log(Logger::WARNING, "Requested block length is too big (we don't support IP fragmentation)\n");
		DropConnection(id, socket);
		return true;
	}

	//g_log("Setup done, asking client to open streams\n");

	return true;
}

void Iperf3Server::OnJsonConfigField(int id, const char* name, const char* value)
{
	//UDP mode
	if(!strcmp(name, "udp"))
	{
		if(!strcmp(value, "true"))
			m_state[id].m_mode = IperfConnectionState::MODE_UDP;
		else if(!strcmp(value, "false"))
			m_state[id].m_mode = IperfConnectionState::MODE_TCP;
		else
			g_log(Logger::WARNING, "Unrecognized JSON value %s for UDP mode (expected true or false)\n", value);
	}

	//Reverse mode
	else if(!strcmp(name, "reverse"))
	{
		if(!strcmp(value, "true"))
			m_state[id].m_reverseMode = true;
		else if(!strcmp(value, "false"))
			m_state[id].m_reverseMode = false;
		else
			g_log(Logger::WARNING, "Unrecognized JSON value %s for reverse mode (expected true or false)\n", value);
	}

	//"omit" must always be 0 for now
	else if(!strcmp(name, "omit"))
	{
		if(0 != strcmp(value, "0"))
			g_log(Logger::WARNING, "Unrecognized JSON value %s for omit mode (expected 0)\n", value);
	}

	//Time
	else if(!strcmp(name, "time"))
	{
		m_state[id].m_time = atoi(value);
		if(m_state[id].m_time)
			g_log("Test duration: %u sec\n", m_state[id].m_time);
		else
			g_log(Logger::WARNING, "Test duration not specified\n");
	}

	//"parallel" must always be 1 for now
	else if(!strcmp(name, "parallel"))
	{
		if(0 != strcmp(value, "1"))
			g_log(Logger::WARNING, "Unrecognized JSON value %s for parallel mode (expected 1)\n", value);
	}

	//For now, ignore blockcount

	//Length
	else if(!strcmp(name, "len"))
	{
		m_state[id].m_len = atoi(value);
		g_log("Block length: %u bytes\n", m_state[id].m_len);
	}

	//Bandwidth limit
	else if(!strcmp(name, "bandwidth"))
	{
		m_state[id].m_bandwidth = atoi(value);
		g_log("Bandwidth: %u Mbps\n", m_state[id].m_bandwidth / (1024*1024));
	}

	//Ignore pacing_timer and client_version
}

#ifdef HAVE_ITCM
__attribute__((section(".tcmtext")))
#endif
void Iperf3Server::Iteration()
{
	//Check if any of our sockets are in TEST_RUNNING
	for(size_t i=0; i<MAX_IPERF_CLIENTS; i++)
	{
		if(!m_state[i].m_valid)
			return;

		//If we're in START state, send a single packet then go to RUNNING state
		switch(m_state[i].m_state)
		{
			case IperfConnectionState::TEST_START:
				SendDataOnStream(i, m_state[i].m_socket);
				m_state[i].m_state = IperfConnectionState::TEST_RUNNING;
				SendState(i, m_state[i].m_socket);
				break;

			case IperfConnectionState::TEST_RUNNING:
				SendDataOnStream(i, m_state[i].m_socket);
				break;

			default:
				break;
		}
	}
}

#ifdef HAVE_ITCM
__attribute__((section(".tcmtext")))
#endif
void Iperf3Server::SendDataOnStream(int id, TCPTableEntry* socket)
{
	auto upack = m_udp.GetTxPacket(socket->m_remoteIP);
	if(!upack)
		return;

	uint32_t len = m_state[id].m_len;
	FillPacket(id, reinterpret_cast<uint32_t*>(upack->Payload()), len);
	m_udp.SendTxPacket(upack, IPERF3_PORT, m_state[id].m_clientPort, len);
}

#ifdef HAVE_ITCM
__attribute__((section(".tcmtext")))
#endif
void Iperf3Server::FillPacket(int id, uint32_t* payload, uint32_t len)
{
	uint32_t wordlen = len/4;
	if(len % 4)
		wordlen ++;

	//Fill seconds and nanoseconds using our timer
	auto countval = g_logTimer.GetCount();
	auto sec = countval / 10000;
	auto ticks = (countval % 10000);
	auto us = ticks * 100;
	payload[0] = __builtin_bswap32(sec);
	payload[1] = __builtin_bswap32(us);

	//Sequence number (for now only 32 bit)
	//Increment first so sequence numbers in packet can be one-based
	uint32_t seq = ++m_state[id].m_sequence;
	payload[2] = __builtin_bswap32(seq);
	payload[3] = 0;

	//fill rest of packet with garbage
	#pragma GCC unroll 4
	for(uint32_t i=4; i<wordlen; i++)
		payload[i] = i;
}
