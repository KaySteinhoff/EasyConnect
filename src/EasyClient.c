#include "../headers/EasyConnect.h"

struct EasyClient client;

int ecCreateClient(uint32_t hostaddress, uint32_t port, int dataLength)
{	
	/*char portStr[10], addrStr[10];
	sprintf(portStr, "%d", port);
	sprintf(addrStr, "%d", hostaddress);*/
	
	client.sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(client.sockfd == -1)
	{
		AppendToLog(ERR_CLIENT_SOCK);
		return 0;
	}
	
	client.poll.fd = client.sockfd;
	client.poll.events = POLLIN;
	
	client.dataLength = dataLength;
	client.data = malloc(dataLength);
	
	client.running = 1;
	
	client.host_addr.sin_family = AF_INET;
	client.host_addr.sin_addr.s_addr = htonl(hostaddress);
	client.host_addr.sin_port = htons(port);

	return 1;
}

void ecDisconnect(void)
{
	close(client.sockfd);
}

int ecClientPollEvents(void)
{
	int num_polls = poll(&client.poll, 1, 0);
		
	if(num_polls == -1)
	{
		AppendToLog(ERR_POLL);
		return 0;
	}

	if(!(client.poll.revents & POLLIN))
		return 1;

	int nbytes = recv(client.sockfd, client.data, client.dataLength, 0);

	if(nbytes <= 0)
	{
		if(nbytes == 0)
		{
			if(client.ConnectionClosedCallback != 0)
				client.ConnectionClosedCallback();
			return 0;
		}

		AppendToLog(ERR_FAULTY_DATA);
		return 0;
	}

	if(client.DataReceivedCallback != 0)
		client.DataReceivedCallback(client.data);
	
	return 1;
}

int ecConnect(void)
{
	if(connect(client.sockfd, (struct sockaddr*)&client.host_addr, sizeof(client.host_addr))<0)
		return 0;

	return 1;
}

int ecSend(void* data)
{
	if(send(client.sockfd, data, client.dataLength, 0) == -1)
	{
		AppendToLog(ERR_PACKAGE_SEND);
		return 0;
	}
	
	return 1;
}

void ecClientClosedCallback(void (*func)())
{
	client.ConnectionClosedCallback = func;
}

void ecClientDataCallback(void (*func)(void*))
{
	client.DataReceivedCallback = func;
}