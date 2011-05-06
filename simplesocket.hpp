/*****************************************************************************\
*                            In the name of God                               *
*******************************************************************************
* SimpleSocket 0.9                                                            *
*                                                                             *
* A C++ library for socket programming intended to be:                        *
*    * simple to use                                                          *
*    * powerful                                                               *
*    * with negligible overhead                                               *
*                                                                             *
*******************************************************************************
* This program is free software; you can redistribute it and/or modify it     *
* under the terms of the GNU General Public License version 3 as published by *
* the Free Software Foundation.                                               *
*                                                                             *
* This program is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   *
* more details.                                                               *
*                                                                             *
* You can get a copy of the GNU General Public License from                   *
* http://www.gnu.org/copyleft/gpl.html                                        *
******************************************************************************* 
* Copyright (C) 2008, 2009, Ebrahim Mohammadi                                 *
* E-mail: ebrahim at mohammadi dot ir                                         *
\*****************************************************************************/

#ifndef _SIMPLESOCKET_HPP_
#define _SIMPLESOCKET_HPP_

#define SIMPLESOCKET_VERSION_MAJOR 1
#define SIMPLESOCKET_VERSION_MINOR 0

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <resolv.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <stdint.h>
using std::string;

enum SocketState
{
	SS_ERROR = -1,   // socket is not open
	SS_READY,        // socket is open and ready to establish a connection (TCP client) or listen (TCP server) or send and receive datagrams (UDP socket)
	SS_CONNECTED,    // a connection is established
	SS_LISTENING,    // server is listening
};

/*********************\
*       Socket        *
\*********************/

class Socket
{
public:
	Socket(int type, int protocol = 0)
	{
		memset(&address, 0, sizeof(address));		// Clear structure
		sd = socket(PF_INET, type, protocol);
		if (sd < 0)
			state = SS_ERROR;
		else
		{
			// Enable address reuse
			int on = 1;
			setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
			state = SS_READY;
		}
	}
	Socket(bool)
	: sd(-1)
	, state(SS_ERROR)
	{
		memset(&address, 0, sizeof(address));		// Clear structure
	}
	~Socket()
	{
		close();
	}
	int close()
	{
		if (sd < 0)
			return -1;
		int res = ::close(sd);
		if (res == 0)
		{
			memset(&address, 0, sizeof(address));		// Clear structure
			state = SS_READY;
		}
		else
			state = SS_ERROR;
		return res;
	}
	uint32_t getIP()
	{
		return address.sin_addr.s_addr;
	}
	char* getIPString(char* ip)		// Allocation is up to user
	{
		return strcpy(ip, inet_ntoa(address.sin_addr));
	}
	uint32_t getLocalIP()
	{
		sockaddr_in adr;
		socklen_t len = sizeof(adr);
		if (getsockname(sd, (sockaddr*) &adr, &len) < 0)
			return 0;
		return adr.sin_addr.s_addr;
	}
	char* getLocalIPString(char* ip)		// Allocation is up to user
	{
		in_addr adr;
		adr.s_addr = getLocalIP();
		return strcpy(ip, inet_ntoa(adr));
	}
	uint16_t getPort()
	{
		return ntohs(address.sin_port);
	}
	SocketState getState()
	{
		return state;
	}
	int getSocketDescriptor()
	{
		return sd;
	}
	int setTimeOut(int sec, int usec = 0)
	{
		timeval t;
		t.tv_sec = sec;
		t.tv_usec = usec;
		return setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(t))
		     + setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
	}
	int setSendTimeOut(int sec, int usec = 0)
	{
		timeval t;
		t.tv_sec = sec;
		t.tv_usec = usec;
		return setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(t));
	}
	int setReceiveTimeOut(int sec, int usec = 0)
	{
		timeval t;
		t.tv_sec = sec;
		t.tv_usec = usec;
		return setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));
	}
	int enableBroadcast()
	{
		int yes = 1;
		return setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
	}
	int disableBroadcast()
	{
		int yes = 0;
		return setsockopt(sd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes));
	}

	static uint32_t getIP(const char* hostname)
	{
		if (hostname == NULL)
			return 0;
		hostent* host = gethostbyname(hostname);
		if (host == NULL)		// gethostbyname failed on hostname
			return 0;
		return *( (uint32_t*) host->h_addr_list[0] );
	}
	static char* getIPString(const char* hostname, char* ip)		// Allocation is up to user
	{
		in_addr adr;
		adr.s_addr = getIP(hostname);
		return strcpy(ip, inet_ntoa(adr));
	}
	static sockaddr_in getAddress(const char* hostname, uint16_t port)
	{
		return getAddress(getIP(hostname), port);
	}
	static sockaddr_in getAddress(uint32_t ip, uint16_t port)
	{
		sockaddr_in res;
		memset(&res, 0, sizeof(res));		// Clear struct
		res.sin_family = PF_INET;
		res.sin_port = htons(port);
		res.sin_addr.s_addr = ip;
		return res;
	}
	
protected:
	int sd;
	sockaddr_in address;
	SocketState state;
};

/*********************\
*      TcpSocket      *
\*********************/

class TcpSocket : public Socket
{
public:
	TcpSocket()
	: Socket(SOCK_STREAM)
	{
	}
	TcpSocket(bool)
	: Socket(true)
	{
	}
};

/*********************\
*      TcpClient      *
\*********************/

class TcpClient : public TcpSocket
{
	friend class TcpServer;
public:
	TcpClient()
	{
	}
	TcpClient(bool)
	: TcpSocket(true)
	{
	}
	int connect(const char* host, uint16_t port)
	{
		if (state != SS_READY)
			return -1;
		return connect(getAddress(host, port));
	}
	int connect(uint32_t ip, uint16_t port)
	{
		if (state != SS_READY)
			return -1;
		return connect(getAddress(ip, port));
	}
	int connect(const sockaddr_in& dest)
	{
		if (state != SS_READY)
			return -1;
		int res = ::connect(sd, (const sockaddr*) &dest, sizeof(dest));
		if (res == 0)
		{
			state = SS_CONNECTED;
			address = dest;
		}
		return res;
	}
	int send(const char* str)
	{
		return send(str, strlen(str));
	}
	int send(const void* data, unsigned int size, int flags = MSG_NOSIGNAL)
	{
		if (state != SS_CONNECTED)
			return -1;
		int res = 0;
		while (size > 0)
		{
			int wrote = ::send(sd, data, size, flags);
			if (wrote <= 0)
			{
				if (res == 0)
					res = -1;
				state = SS_ERROR;
				break;
			}
			data = (const char*) data + wrote;
			size -= wrote;
			res += wrote;
		}
		return res;
	}
	int receive(void* buf, unsigned int size, int flags = MSG_WAITALL)
	{
		if (state != SS_CONNECTED)
			return -1;
		int res = 0;
		while (size > 0)
		{
			int read_size = recv(sd, buf, size, flags);
			if (read_size <= 0)
			{
				if (res == 0)
					res = -1;
				state = SS_ERROR;
				break;
			}
			buf = (char*) buf + read_size;
			size -= read_size;
			res += read_size;
		}
		return res;
	}
	int readLine(string& line, unsigned int maxSize = (unsigned int) -1, char delimiter = '\n', char ignore = '\r')
	{
		while (maxSize--)
		{
			char c;
			int readSize = read(sd, &c, 1);
			if (readSize <= 0)
			{
				state = SS_ERROR;
				return -2;
			}
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
	TcpServer()
	{
	}
	TcpServer(bool)
	: TcpSocket(true)
	{
	}
	int listen(uint16_t port, const char* host = NULL, int backlog = 32)
	{
		if (state != SS_READY)
			return -1;
		sockaddr_in server;
		if (host)
			server = getAddress(host, port);
		else
			server = getAddress(htonl(INADDR_ANY), port);
		if (bind(sd, (const sockaddr*) &server, sizeof(server)) != 0)
			return -1;
		int res = ::listen(sd, backlog);
		if (res == 0)
		{
			state = SS_LISTENING;
			address = server;
		}
		return res;	
	}
	int accept(TcpClient& client)
	{
		if (state != SS_LISTENING)
			return -1;
		client.close();
		unsigned int addrSize = sizeof(client.address);
		client.sd = ::accept(sd, (sockaddr*) &client.address, &addrSize);
		if (client.sd < 0)
			return -1;
		client.state = SS_CONNECTED;
		return 0;
	}
	TcpClient* accept()
	{
		if (state != SS_LISTENING)
			return NULL;
		TcpClient* client = new TcpClient(true);
		if (accept(*client))
		{
			delete client;
			return NULL;
		}
		return client;
	}
};

/*********************\
*      UdpSocket      *
\*********************/

class UdpSocket : public Socket
{
public:
	UdpSocket()
	: Socket(SOCK_DGRAM)
	{
	}
	UdpSocket(bool)
	: Socket(true)
	{
	}
	int bind(uint16_t port, const char* host = NULL)
	{
		if (state != SS_READY)
			return -1;
		sockaddr_in server;
		if (host)
			server = getAddress(host, port);
		else
			server = getAddress(htonl(INADDR_ANY), port);
		return ::bind(sd, (const sockaddr*) &server, sizeof(server));
	}
	int connect(const char* host, uint16_t port)		// Set default target for target-less send and receive
	{
		if (state != SS_READY)
			return -1;
		return connect(getAddress(host, port));
	}
	int connect(uint32_t ip, uint16_t port)
	{
		if (state != SS_READY)
			return -1;
		return connect(getAddress(ip, port));
	}
	int connect(const sockaddr_in& dest)		// Set default target for target-less send and receive
	{
		if (state != SS_READY)
			return -1;
		int res = ::connect(sd, (const sockaddr*) &dest, sizeof(dest));
		if (res == 0)
			address = dest;
		return res;
	}
	int receive(void* buf, unsigned int size, int flags = MSG_WAITALL)
	{
		if (state != SS_READY)
			return -1;
		int res = recv(sd, buf, size, flags);
		if (res < 0)
			state = SS_ERROR;
		return res;
	}
	int receive(void* buf, unsigned int size, sockaddr_in& address, int flags = MSG_WAITALL)
	{
		if (state != SS_READY)
			return -1;
		socklen_t len = sizeof(address);
		int res = recvfrom(sd, buf, size, flags, (sockaddr*)&address, &len);
		if (res < 0)
			state = SS_ERROR;
		return res;
	}
	int send(const void* data, unsigned int size, int flags = MSG_NOSIGNAL)
	{
		if (state != SS_READY)
			return -1;
		int res = 0;
		while (size > 0)
		{
			int wrote = ::send(sd, data, size, flags);
			if (wrote < 0)
			{
				if (res == 0)
					res = -1;
				state = SS_ERROR;
				break;
			}
			data = (const char*) data + wrote;
			size -= wrote;
			res += wrote;
		}
		return res;
	}
	int send(const void* data, unsigned int size, const sockaddr_in& address, int flags = MSG_NOSIGNAL)
	{
		if (state != SS_READY)
			return -1;
		int res = 0;
		while (size > 0)
		{
			int wrote = ::sendto(sd, data, size, flags, (sockaddr*) &address, sizeof(address));
			if (wrote < 0)
			{
				if (!res)
					res = -1;
				state = SS_ERROR;
				break;
			}
			data = (const char*) data + wrote;
			size -= wrote;
			res += wrote;
		}
		return res;
	}
};

#endif // _SIMPLESOCKET_HPP_
