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

#ifndef _FTP_SERVER_H_
#define _FTP_SERVER_H_

#include "simplesocket.hpp"
#include <fcntl.h>		// For O_RDONLY, etc.
#include <unistd.h>		// For getcwd, chdir
#include <cassert>
#include <string>
using std::string;

#define MAX_COMMAND_LEN 4096
#define FILE_BUF_LEN 32768

#define DEFAULT_FILE_PERM 0640
#define DEFAULT_DIR_PERM 0750

class FtpServer
{
	enum State
	{
		STATE_NEED_USER,
		STATE_NEED_PASS,
		STATE_READY,
		STATE_CONNECTED,
		STATE_BUSY,
	};

public:
	FtpServer(TcpClient& _client, const char* _root)
	: client(_client)
	, root(_root)
	{
	}

	int serve()
	{
		chdir(root.c_str());
		if (client.send("220 Salaam!\r\n") < 0)
		{
			fprintf(stderr, "Error: Unable to send data to client\n");
			return 1;
		}
		state = STATE_NEED_USER;
		string username;
		bool passive = true;
		uint32_t ip = 0;
		uint32_t me = client.getLocalIP();
		uint16_t port = 0;
		uint16_t myPort = rand() % (65536 - 1024) + 1024;
		for (;;)
		{
			string command;
			if (client.readLine(command, MAX_COMMAND_LEN))
			{
				client.send("421 Command line too long\r\n");
				break;
			}
			string cmd = command.substr(0, command.find_first_of(" \r\n"));
			for (int i = cmd.size() - 1; i >= 0; --i)
				cmd[i] = toupper(cmd[i]);
			string parameters = command.substr(cmd.size());
			size_t tmp = parameters.find_first_not_of(' ');
			if (tmp < parameters.length())
				parameters = parameters.substr(tmp);
			tmp = parameters.find_last_not_of(' ');
			if (tmp < parameters.length())
				parameters = parameters.substr(0, tmp + 1);
			fprintf(stderr, "cmd: %s, parameters: %s\n", cmd.c_str(), parameters.c_str());
			if (cmd == "QUIT")
			{
				client.send("221 Khoda Negahdaar!\r\n");
				break;
			}
			switch (state)
			{
				case STATE_NEED_USER:
					if (cmd != "USER")
					{
						client.send("530 Login please\r\n");
						break;
					}
					username = parameters;
					state = STATE_NEED_PASS;
					client.send("331 Please specify the password\r\n");
					break;
				case STATE_NEED_PASS:
					if (cmd != "PASS")
					{
						client.send("530 Login please\r\n");
						break;
					}
					if (username != "anonymous")
					{
						client.send("530 Authentication failed\r\n");
						state = STATE_NEED_USER;
						break;
					}
					client.send("230 Login successful\r\n");
					state = STATE_READY;
					break;
				case STATE_READY:
					if (cmd == "PORT")
					{
						if (parsePort(parameters, ip, port))
						{
							client.send("501 Syntax error in parameters or arguments\r\n");
							break;
						}
						fprintf(stderr, "PORT -> Host: %u.%u.%u.%u, Port: %u\n",
								*(uint8_t*)&ip, *(1+(uint8_t*)&ip), *(2+(uint8_t*)&ip), *(3+(uint8_t*)&ip), port);
						passive = false;
						client.send("200 PORT command successful\r\n");
						state = STATE_CONNECTED;
					}
					else if (cmd == "PASV")
					{
						char buf[MAX_COMMAND_LEN];
						if (dataServerSocket.getState() != SS_LISTENING)
						{
							fprintf(stderr, "PASV -> Host: %u.%u.%u.%u, Port: %u\n",
									*(uint8_t*)&me, *(1+(uint8_t*)&me), *(2+(uint8_t*)&me), *(3+(uint8_t*)&me), myPort);
							if (dataServerSocket.listen(myPort) < 0)
							{
								client.send("550 Failed to listen for data connection\r\n");
								break;
							}
						}
						sprintf(buf, "227 Entering Passive Mode (%hhu,%hhu,%hhu,%hhu,%hhu,%hhu)\r\n",
								*(uint8_t*)&me, *(1+(uint8_t*)&me), *(2+(uint8_t*)&me), *(3+(uint8_t*)&me), *(1+(uint8_t*)&myPort), *(uint8_t*)&myPort);
						client.send(buf);
						passive = true;
						state = STATE_CONNECTED;
					}
					else if (cmd == "PWD")
					{
						char buf[MAX_COMMAND_LEN];
						if (!getcwd(buf, MAX_COMMAND_LEN))
						{
							client.send("550 Failed to get current working directory\r\n");
							break;
						}
						client.send("257 \"");
						client.send(buf);
						client.send("\"\r\n");
					}
					else if (cmd == "CWD")
					{
						if (chdir(parameters.c_str()) < 0)
						{
							client.send("550 Failed to change directory\r\n");
							break;
						}
						client.send("250 Directory successfully changed\r\n");
					}
					else if (cmd == "CDUP")
					{
						if (chdir("..") < 0)
						{
							client.send("550 Failed to change directory\r\n");
							break;
						}
						client.send("250 Directory successfully changed\r\n");
					}
					else if (cmd == "MKD")
					{
						if (mkdir(parameters.c_str(), DEFAULT_DIR_PERM) < 0)
						{
							client.send("550 Failed to create directory\r\n");
							break;
						}
						client.send("250 Directory successfully created\r\n");
					}
					else if (cmd == "RMD")
					{
						if (rmdir(parameters.c_str()) < 0)
						{
							client.send("550 Failed to remove directory\r\n");
							break;
						}
						client.send("250 Directory successfully removed\r\n");
					}
					else if (cmd == "DELE")
					{
						if (unlink(parameters.c_str()) < 0)
						{
							client.send("550 Failed to remove file\r\n");
							break;
						}
						client.send("250 File successfully removed\r\n");
					}
					else if (cmd == "SYST")
						client.send("215 UNIX Type: L8\r\n");
					else if (cmd =="TYPE")
						client.send("200 Switched mode\r\n");
					else if (cmd == "RETR" || cmd == "LIST" || cmd == "STOR")
						client.send("425 Use PORT or PASV first\r\n");
					else if (cmd == "NOOP")
						client.send("200 NOOP-ing!\r\n");
					else
						client.send("500 Unknown command\r\n");
					break;
				case STATE_CONNECTED:
				{
					if (cmd != "RETR" && cmd != "LIST" && cmd != "STOR")
						client.send("500 Unknown command\r\n");
					TcpClient dataSocket;
					client.send("150 Opening data connection\r\n");
					if (passive)
					{
						if (dataServerSocket.accept(dataSocket) < 0)
						{
							client.send("425 Failed to open data connection\r\n");
							break;
						}
					}
					else
					{
						if (dataSocket.connect(ip, port) < 0)
						{
							client.send("425 Failed to open data connection\r\n");
							break;
						}
					}
					state = STATE_READY;
					if (cmd == "RETR")
					{
						int fd = open(parameters.c_str(), O_RDONLY);
						if (fd < 0)
						{
							client.send("550 Failed to open file\r\n");
							break;
						}
						char buf[FILE_BUF_LEN];
						for (;;)
						{
							ssize_t readSize = read(fd, buf, FILE_BUF_LEN);
							if (readSize < 0)
							{
								client.send("426 Failure reading file\r\n");
								break;
							}
							if (dataSocket.send(buf, readSize) != readSize)
							{
								client.send("426 Failure writing to network socket\r\n");
								break;
							}
							if (readSize < FILE_BUF_LEN)
								break;
						}
						client.send("226 File sent successfully\r\n");
					}
					else if (cmd == "STOR")
					{
						int fd = open(parameters.c_str(), O_WRONLY | O_CREAT, DEFAULT_FILE_PERM);
						if (fd < 0)
						{
							client.send("550 Failed to open file\r\n");
							break;
						}
						char buf[FILE_BUF_LEN];
						for (;;)
						{
							ssize_t readSize = dataSocket.receive(buf, FILE_BUF_LEN);
							if (readSize < 0)
							{
								client.send("426 Failure reading from network socket\r\n");
								break;
							}
							if (write(fd, buf, readSize) != readSize)
							{
								client.send("426 Failure writing to file\r\n");
								break;
							}
							if (readSize < FILE_BUF_LEN)
								break;
						}
						client.send("226 File stored successfully\r\n");
					}
					else if (cmd == "LIST")
					{
						FILE* ls = popen(("ls -l --time-style=\"+%b %d %Y\" " + parameters).c_str(), "r");
						if (!ls)
						{
							client.send("550 Failed to get file listing\r\n");
							break;
						}
						char buf[FILE_BUF_LEN];
						for (;;)
						{
							ssize_t readSize = fread(buf, 1, FILE_BUF_LEN, ls);
							if (dataSocket.send(buf, readSize) != readSize)
							{
								client.send("426 Failure writing to network socket\r\n");
								break;
							}
							if (readSize < FILE_BUF_LEN)
								break;
						}
						client.send("226 Listing sent successfully\r\n");
						pclose(ls);
					}
					break;
				}
				case STATE_BUSY:
					break;
				default:
					assert(false);
			}
		}
		return 0;
	}

	// Fields
	TcpClient& client;
	string root;
	State state;
	TcpServer dataServerSocket;

private:
	bool parsePort(const string& str, uint32_t& ip, uint16_t& port)
	{
		if (sscanf(str.c_str(), "%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",
					(uint8_t*)&ip, 1+(uint8_t*)&ip, 2+(uint8_t*)&ip, 3+(uint8_t*)&ip,
					1+(uint8_t*)&port, (uint8_t*)&port) != 6)
			return true;
		return false;
	}
};

#endif // _FTP_SERVER_H_
