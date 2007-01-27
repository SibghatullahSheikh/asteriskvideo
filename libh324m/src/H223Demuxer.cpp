/* H324M library
 *
 * Copyright (C) 2006 Sergio Garcia Murillo
 *
 * sergio.garcia@fontventa.com
 * http://sip.fontventa.com
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <string.h>
#include "H223Demuxer.h"
#define NONE  0
#define HEAD  1
#define PDU   2


H223Demuxer::H223Demuxer()
{
}

H223Demuxer::~H223Demuxer()
{
}

int H223Demuxer::SetChannel(int num,H223ALReceiver *receiver)
{
	//Save the reciever
	al[num] = receiver;

	return 1;
}

int H223Demuxer::Open(H223MuxTable *table)
{
	//The table
	mux = table;

	//Reset state
	state= NONE;

	//Reset flags
	flag.Clear();
	begin.Clear();

	//And header
	header.Clear();

	return true;
}

int H223Demuxer::Close()
{
	//Log
	Debug("-Close demuxer\n");
	
	//exit
	return 1;
}

void H223Demuxer::StartPDU(H223Flag &flag)
{
	//Copy the flag
	begin = flag;
	//Reset counter
	counter = 0;
	//No channel
	channel = -1;
}

void H223Demuxer::EndPDU(H223Flag &flag)
{
	//If the flag is the complement
	if (flag.complement)
		//if there is channel
		if ((channel!=-1) && (al[channel]!=NULL))
			//Send the closing pdu to the last channel
			al[channel]->SendClosingFlag();
}

void H223Demuxer::Demultiplex(BYTE b)
{
	//Depending on the state
	switch(state)
	{
		case NONE:
			//Append the byte to the flag
			flag.Append(b);
			
			//If we have a full header
			if (!flag.IsValid())
				return;
	
			//Start the PDU
			StartPDU(flag);

			//Clear flag
			flag.Clear();

			//Change state
			state = HEAD;
			
			break;
		case HEAD:
			//Append the byte to the header
			header.Append(b);

			//If still don't we have a complete header
			if (!header.IsComplete())
				return;
	
			//Is the header correct?
			if (!header.IsValid())
			{
				//Clean the header
				header.Clear();
				//Reset state
				state = NONE;
			}

			//We have a good header go for the pdu
            state = PDU;

			break;
		case PDU:
			//Check if the buffer of the flag is full or not
			int complete = flag.IsComplete();

			//Append the byte to the flag and get previous one
			BYTE  a = flag.Append(b);

			//Send the byte to the corresponding AL
			if (complete)
				Send(a);

			//If we have a good flag
			if (!flag.IsValid())
				//And return
				return;
			
			//Set end PDU
			EndPDU(flag);

			//Start the next PDU
			StartPDU(flag);

			//Clear flag
			flag.Clear();

			//Clear the header 
			header.Clear();

			//Change state
			state = HEAD;
			break;
	}
}


void H223Demuxer::Send(BYTE b)
{
	//Get the next channel from the mux table
	channel = mux->GetChannel(header.mc,counter++);

	//Send it to the al
	if (channel!=-1)
		al[channel]->Send(b);
}
