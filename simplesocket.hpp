#if 0
*******************************************************************************
*                            In the name of God
*******************************************************************************
* SimpleSocket 0.1
*
* A C++ library for socket programming intended to be:
*    * simple to use
*    * powerful
*    * with negligible overhead
*
*******************************************************************************
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License version 3 as published by
* the Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
* more details.
* 
* You can get a copy of the GNU General Public License from
* http://www.gnu.org/copyleft/gpl.html
******************************************************************************* 
* Copyright (C) 2008, Mohammad Ebrahim Mohammadi Panah
* E-mail: mebrahim  at  gmail dot com
*******************************************************************************
#endif

#ifndef _SIMPLESOCKET_HPP_
#define _SIMPLESOCKET_HPP_

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <resolv.h>
#include <unistd.h>
#include <string.h>
#include <string>
using std::string;

typedef unsigned short Port;

enum SocketState
{
	SS_ERROR = -1,   // socket is not open
	SS_READY,        // socket is open and ready to establish a connection (client) or listen (server)
	SS_CONNECTED,    // a connection is established
	SS_LISTENING,    // server is listening
};

/*********************\
*      TcpSocket      *
\*********************/

class TcpSocket
{
public:
	TcpSocket()
	{
		memset(&address, 0, sizeof(address));		// Clear structure
		sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sd < 0)
			state = SS_ERROR;
		else
			state = SS_READY;
	}
	~TcpSocket()
	{
		close();
	}
	
	int close()
	{
		if (state == SS_ERROR)
			return -2;
		int res = ::close(sd);
		if (res == 0)
		{
			memset(&address, 0, sizeof(address));		// Clear structure
			state = SS_READY;
		}
		return res;
	}
	char* getIP(char* ip)		// Allocation is up to user!
	{
		return strcpy(ip, inet_ntoa(address.sin_addr));
	}
	Port getPort()
	{
		return ntohs(address.sin_port);
	}
	SocketState getState()
	{
		return state;
	}

	static char* getIP(const char* hostname, char* ip)		// Allocation is up to user!
	{
		return strcpy(ip, inet_ntoa(getAddr(hostname)));
	}
	
protected:
	int sd;
	sockaddr_in address;
	SocketState state;
	
	static in_addr getAddr(const char* hostname)
	{
		in_addr hostAddr = {0};

		if (hostname == NULL)
			return hostAddr;

		hostent* host = gethostbyname(hostname);

		if (host == NULL)		// gethostbyname failed on hostname
			return hostAddr;
		
		hostAddr.s_addr = *( (unsigned long*) host->h_addr_list[0] );
		
		return hostAddr;
	}
};

/*********************\
*      TcpClient      *
\*********************/

class TcpClient : public TcpSocket
{
	friend class TcpServer;
public:
	int connect(const char* host, Port port)
	{
		if (state != SS_READY)
			return -2;
		
		sockaddr_in dest;
		memset(&dest, 0, sizeof(dest));		// Clear struct
		dest.sin_family = PF_INET;
		dest.sin_port = htons(port);
		dest.sin_addr = getAddr(host);

		int res = ::connect(sd, (const sockaddr*) &dest, sizeof(dest));
		if (res == 0)
		{
			state = SS_CONNECTED;
			address = dest;
		}
		return res;
	}
	int disconnect()
	{
		return TcpSocket::close();
	}
	int send(const char* data)
	{
		return send(data, strlen(data));
	}
	int send(const void* data, unsigned int size, int flags = MSG_NOSIGNAL)
	{
		if (state != SS_CONNECTED)
			return -2;
		
		int res = 0;
		while (size > 0)
		{
			int wrote = ::send(sd, data, size, flags);
			if (wrote < 0)
			{
				res = -4;
				break;
			}
			data = (const char*) data + wrote;
			size -= wrote;
		}
		return res;
	}
	
	char* receive(unsigned int size)		// delete is up to user!
	{
		char* buf = new char[size+1];
		if (receive(buf, size) < 0)
		{
			delete [] buf;
			return NULL;
		}
		return buf;
	}
	int receive(void* buf, unsigned int size, int flags = MSG_WAITALL)
	{
		if (state != SS_CONNECTED)
			return -2;
		
		if (recv(sd, buf, size, flags) != size)
			return -4;
		return 0;
	}
	int readLine(string& line, unsigned int maxSize = (unsigned int) -1, char delimiter = '\n', char ignore = '\r')
	{
		while (maxSize--)
		{
			char c;
			int readSize = read(sd, &c, 1);
			if (readSize < 0)
				return -2;
			if (readSize == 0)
				break;
			if (c == delimiter)
				return 0;
			if (c != ignore)
				line += c;
		}
		return 1;
	}
};

/*********************\
*      TcpServer      *
\*********************/

class TcpServer : public TcpSocket
{
public:
	int listen(Port port, int backlog = 32)
	{
		if (state != SS_READY)
			return -2;
		
		sockaddr_in server;
		memset(&server, 0, sizeof(server));		// Clear struct
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = htonl(INADDR_ANY);   // Incoming address
		server.sin_port = htons(port);       // Server port

		int res = bind(sd, (const sockaddr*) &server, sizeof(server));
		
		if (res != 0)
			return -3;
		
		res = ::listen(sd, backlog);

		if (res == 0)
		{
			state = SS_LISTENING;
			address = server;
		}
		
		return res;	
	}
	int accept(TcpClient& client)
	{
		if (state != SS_LISTENING || client.state != SS_READY)
			return -2;

		unsigned int addrSize = sizeof(client.address);
		client.sd = ::accept(sd, (sockaddr*) &client.address, &addrSize);
		
		if (client.sd < 0)
			return -3;
		
		client.state = SS_CONNECTED;
		return 0;
	}
	TcpClient* accept()
	{
		if (state != SS_LISTENING)
			return NULL;
		
		TcpClient* client = new TcpClient;
		if (accept(*client))
		{
			delete client;
			return NULL;
		}
		return client;
	}
	int stop()
	{
		return TcpSocket::close();
	}
};

#endif // _SIMPLESOCKET_HPP_
