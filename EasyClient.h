struct EasyClient
{
	void* data;
	int dataLength;
	int sockfd;
	struct sockaddr_in host_addr;
	
	int running;
	
	struct pollfd poll;
	
	void (*ConnectionClosedCallback)();
	void (*DataReceivedCallback)(void* data);
	void (*Update)();
};

struct EasyClient ecCreateClient(uint32_t hostaddress, uint32_t port, int dataLength)
{
	struct EasyClient client;

	client.sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(client.sockfd == -1)
	{
		AppendToLog(ERR_CLIENT_SOCK);
		return client;
	}
	
	client.poll.fd = client.sockfd;
	client.poll.events = POLLIN;
	
	client.dataLength = dataLength;
	client.data = malloc(dataLength);
	
	client.running = 1;
	
	client.host_addr.sin_family = AF_INET;
	client.host_addr.sin_addr.s_addr = htonl(hostaddress);
	client.host_addr.sin_port = htons(port);

	return client;
}

void ecDisconnectClient(struct EasyClient* client)
{
	client->running = 0;
	close(client->sockfd);
}

int ecConnectClient(struct EasyClient* client)
{
	if(client->Update == 0)
	{
		AppendToLog(ERR_NO_UPDATE);
		return 0;
	}

	if(connect(client->sockfd, (struct sockaddr*)&client->host_addr, sizeof(client->host_addr))<0)
		return 0;

	while(client->running)
	{
		int num_polls = poll(&client->poll, 1, 1);
		
		if(num_polls == -1)
		{
			AppendToLog(ERR_POLL);
			return 0;
		}
				
		if(!(client->poll.revents & POLLIN))
		{
			client->Update();
			continue;
		}
		
		int nbytes = recv(client->sockfd, client->data, client->dataLength, 0);

		if(nbytes <= 0)
		{
			if(nbytes == 0)
			{
				if(client->ConnectionClosedCallback != 0)
					client->ConnectionClosedCallback();
				break;
			}
			
			AppendToLog(ERR_FAULTY_DATA);
			return 0;
		}

		if(client->DataReceivedCallback != 0)
			client->DataReceivedCallback(client->data);
		
		client->Update();
	}

	return 1;
}

int ecSend(struct EasyClient client, void* data)
{
	if(send(client.sockfd, data, client.dataLength, 0) == -1)
	{
		AppendToLog(ERR_PACKAGE_SEND);
		return 0;
	}
	
	return 1;
}

void ecClientClosedCallback(struct EasyClient* client, void (*func)())
{
	client->ConnectionClosedCallback = func;
}

void ecClientDataCallback(struct EasyClient* client, void (*func)(void*))
{
	client->DataReceivedCallback = func;
}

void ecClientUpdate(struct EasyClient* client, void (*func)())
{
	client->Update = func;
}
