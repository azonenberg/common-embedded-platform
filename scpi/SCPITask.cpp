/***********************************************************************************************************************
*                                                                                                                      *
* common-embedded-platform                                                                                             *
*                                                                                                                      *
* Copyright (c) 2024-2026 Andrew D. Zonenberg and contributors                                                         *
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

#include "core/platform.h"
#include "SCPITask.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Construction / destruction

SCPITask::SCPITask(BufferedCharacterDeviceBase* device)
	: m_writepos(0)
	, m_device(device)
{
	Clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SCPITask::Iteration()
{
	while(m_device->HasInput())
		OnChar(m_device->BlockingRead());
}

void SCPITask::OnChar(char ch)
{
	//Backspace
	if( (ch == '\b') || (ch == '\x7f') )
	{
		if(m_writepos > 0)
		{
			m_writepos --;
			//m_device->Printf("\b \b");
		}
	}

	//Newline
	else if( (ch == '\r') || (ch == '\n') )
	{
		m_line[m_writepos] = '\0';
		//m_device->Printf("\n");
		OnLine();
	}

	//Anything else? append it
	else if(isprint(ch))
	{
		//Do we have space? Echo it
		if( (m_writepos + 1) < sizeof(m_line))
		{
			m_line[m_writepos ++] = ch;
			//m_device->Printf("%c", ch);
		}

		//Out of space, process it now
		else
		{
			m_line[m_writepos] = '\0';
			OnLine();
		}
	}
}

void SCPITask::OnLine()
{
	char subject[SCPI_LINE_MAX] = {0};
	char command[SCPI_LINE_MAX] = {0};
	size_t writepos = 0;

	//Skip blank lines
	if(m_writepos == 0)
		return;

	//Does it start with a *? Special command
	else if(m_line[0] == '*')
		OnSpecialCommand(m_line);

	//Does it end in a question mark? Query
	else if(m_line[m_writepos - 1] == '?')
	{
		bool inCommand = false;
		for(size_t i=0; i<m_writepos-1; i++)
		{
			if(m_line[i] == ':')
			{
				inCommand = true;
				writepos = 0;
			}
			else if(inCommand)
				command[writepos ++] = m_line[i];
			else
				subject[writepos ++] = m_line[i];
		}

		//we got command + subject
		if(inCommand)
			OnQuery(subject, command);
		else
			OnQuery("", subject);
	}

	//No, command
	else
	{
		bool inCommand = false;
		size_t i = 0;
		for(; i<m_writepos; i++)
		{
			if(m_line[i] == ':')
			{
				inCommand = true;
				writepos = 0;
			}
			else if(inCommand)
			{
				if(isspace(m_line[i]))
				{
					i++;
					break;
				}
				else
					command[writepos ++] = m_line[i];
			}
			else
				subject[writepos ++] = m_line[i];
		}

		//we got command + subject and maybe args
		if(inCommand)
			OnCommand(subject, command, m_line + i);
		else
			OnCommand("", subject, m_line + i);
	}

	//Done
	memset(m_line, 0, sizeof(m_line));
	m_writepos = 0;
}
