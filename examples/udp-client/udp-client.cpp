#if 0
*******************************************************************************
*                            In the name of God
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

#include <iostream>
using namespace std;

#include <simplesocket.hpp>

#define SERVER_HOST "localhost"
#define SERVER_PORT 1234
#define BUF_LEN 1024

int
main(int argc, char* argv[])
{
	UdpSocket socket;

	char buf[BUF_LEN];
	while (fgets(buf, BUF_LEN, stdin))
	{
		sockaddr_in adr = Socket::getAddress(SERVER_HOST, SERVER_PORT);
		buf[BUF_LEN-1] = '\0';
		int len = strlen(buf) - 1;
		buf[len] = '\0';		// Remove trailing \n
		sockaddr_in asdr = adr;
		if (socket.send(buf, len, adr) < 0)
		{
			fprintf(stderr, "Error: Failed to write data into socket -> %m\n");
			return 1;
		}
		sockaddr_in srvAddr;
		int readSize = socket.receive(buf, BUF_LEN, srvAddr);
		if (readSize < 0)
		{
			fprintf(stderr, "Error: Failed to read data from socket -> %m\n");
			return 1;
		}
		unsigned char* ip = (unsigned char*) &srvAddr.sin_addr.s_addr;
		printf("%u.%u.%u.%u:%u -> %.*s\n", ip[0], ip[1], ip[2], ip[3], ntohs(srvAddr.sin_port), readSize, buf);
	}
	return 0;
}
