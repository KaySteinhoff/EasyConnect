#include "../headers/EasyConnect.h"

struct EasyClient client;

int ecCreateClient(char* hostaddress, uint32_t port, int dataLength)
{	
	char portStr[10];
	sprintf(portStr, "%d", port);
	
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof(hints));;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if((rv = getaddrinfo(hostaddress, portStr, &hints, &servinfo)) != 0)
	{
		AppendToLog(ERR_NO_SERVER);
		return 0;
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
      if((client.sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
         AppendToLog(ERR_CLIENT_SOCK);
      	continue;
      }
	 		
      if(connect(client.sockfd, p->ai_addr, p->ai_addrlen) == -1) {
         close(client.sockfd);
   		AppendToLog(ERR_CONNECT);
      	continue;
      }

		break;
   }

	 
	if(p == NULL)
		return 0;

   freeaddrinfo(servinfo);
	
	client.poll.fd = client.sockfd;
	client.poll.events = POLLIN;
	
	client.dataLength = dataLength;
	client.data = malloc(dataLength);
	
	client.running = 1;

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