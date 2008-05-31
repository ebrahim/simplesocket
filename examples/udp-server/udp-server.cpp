#include <iostream>
using namespace std;

#include <simplesocket.hpp>

#define PORT 1234
#define BUF_LEN 1024
#define ACK_MSG "Got your message!"

int
main(int argc, char* argv[])
{
	UdpSocket socket;
	if (socket.bind(PORT))
	{
		fprintf(stderr, "Error: Unable to bind to port %u -> %m\n", PORT);
		return 1;
	}

	char buf[BUF_LEN];
	for (;;)
	{
		sockaddr_in adr;
		int readSize = socket.receive(buf, BUF_LEN, adr);
		if (readSize < 0)
		{
			fprintf(stderr, "Error: Failed to read data from socket -> %m\n");
			return 1;
		}
		unsigned char* ip = (unsigned char*) &adr.sin_addr.s_addr;
		printf("%u.%u.%u.%u:%u -> %.*s\n", ip[0], ip[1], ip[2], ip[3], ntohs(adr.sin_port), readSize, buf);
		sleep(1);
		if (socket.send(ACK_MSG, sizeof(ACK_MSG), adr) < 0)
			fprintf(stderr, "Warning: Failed to write data into socket -> %m\n");
	}
	return 0;
}
