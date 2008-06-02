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

#include <syslog.h>
#include "simplesocket.hpp"
#include "ftp-server.hpp"

#define DAEMON_NAME "simple-ftpd"
#define DEFAULT_PORT 2121
#define DEFAULT_BACKLOG 128

int
main(int argc, char* argv[])
{
	// Initialize loggin system
	openlog(DAEMON_NAME, 0, LOG_DAEMON);

	// Get parameters and settings
	uint16_t port = DEFAULT_PORT;
	int backlog = DEFAULT_BACKLOG;
	const char* root;
	if (argc < 2)
		root = ".";
	else
	{
		root = argv[1];
		if (argc > 2)
			port = atoi(argv[2]);
	}

	// Start listening
	TcpServer server;
	if (server.listen(port, backlog))
	{
		syslog(LOG_CRIT, "Unable to listen on port %d: %m", port);
		return 2;
	}
	int res = 0;
	syslog(LOG_NOTICE, "Listening on port %d", port);
	// Main loop of the server
	for (;;)
	{
		// Accept connections
		TcpClient client;
		if (server.accept(client))
		{
			syslog(LOG_ERR, "Unable to accept client: %m");
			res = 3;
			break;
		}
		syslog(LOG_INFO, "Accepted a connection");

		// Create a child to handle this connection and go to accept the next connection in father
		pid_t pid = fork();
		if (pid == 0)
		{
			// In child
			FtpServer ftp(client, root);
			ftp.serve();
			client.close();
			break;
		}
		// In father
	}
	server.close();
	return res;
}
