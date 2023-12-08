#include "../headers/EasyConnect.h"

struct EasyServer server;

int ecadd_addr(struct AddrList* list, struct sockaddr_storage addr)
{
	if(list->length == list->size)
	{
		list->size *= 2;
		list->addrs = (struct sockaddr_storage*)realloc(list->addrs, list->size*sizeof(struct sockaddr_storage));
	}

	list->addrs[list->length] = addr;
	list->length++;

	return 1;
}

int ecadd_pfd(struct PFDList* list, int fd)
{
	if(list->length == list->size)
	{
		list->size *= 2;
		list->pfds = (struct pollfd*)realloc(list->pfds, list->size*sizeof(struct pollfd)); 
	}

	list->pfds[list->length].fd = fd;
	list->pfds[list->length].events = POLLIN;
	list->length++;
	
	return 1;
}

int ecdel_addr(struct AddrList* list, int index)
{
	if(index >= list->length || index < 0)
		return 0;

	if(index != 0)
		list->addrs[index] = list->addrs[list->length-1];
	list->length--;

	return 1;
}

int ecdel_pfd(struct PFDList* list, int index)
{	
	if(index >= list->length || index < 0)
		return 0;

	list->pfds[index] = list->pfds[list->length-1];	
	list->length--;
	
	return 1;
}

int ecCreateServer(char* openaddress, uint32_t port, int socketType, int maxClients, int dataLength)
{
	int rv;
	
	struct addrinfo hints, *ai, *p;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = socketType;
	hints.ai_flags = AI_PASSIVE;
	
	char str[10];
	sprintf(str, "%ld", (long)port);
	
	if((rv = getaddrinfo(openaddress, str, &hints, &ai)) != 0)
	{
		printf("Failed to get address info\n");
		AppendToLog(ERR_SERVER_SOCK);
		return 0;
	}
		
	for(p = ai; p != NULL; p = p->ai_next)
	{
		server.serverfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		
		if(server.serverfd == -1)
			continue;

		if(socketType == TCP)
		{
			int yes = 1;
			setsockopt(server.serverfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		}else if(socketType == UDP)
		{
			struct timeval recv_timeout;
			recv_timeout.tv_usec = 10;
			setsockopt(server.serverfd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));
		}else
		{
			AppendToLog(ERR_NO_PROTOCOL);
			return 0;
		}
		
		if(bind(server.serverfd, p->ai_addr, p->ai_addrlen) < 0)
		{
			close(server.serverfd);
			continue;
		}
		
		break;
	}
	
	freeaddrinfo(ai);
		
	if(p == NULL)
	{
		AppendToLog(ERR_SERVER_SOCK);
		return 0;
	}

	if(socketType == TCP)
	{
		if(listen(server.serverfd, maxClients) < 0)
		{
			AppendToLog(ERR_SERVER_SOCK);
			return 0;
		}
	}
		
	server.maxClients = maxClients;
	server.dataLength = dataLength;
	server.socketType = socketType;
	server.pfdList.size = 1;
	server.addrList.size = 1;
	server.pfdList.length = 0;
	server.addrList.length = 0;
	
	if(socketType == TCP)
		server.pfdList.pfds = (struct pollfd*)malloc(sizeof(struct pollfd));
	else if(socketType == UDP)
		server.addrList.addrs = (struct sockaddr_storage*)malloc(sizeof(struct sockaddr_storage));
	
	server.data = malloc(dataLength);
	server.socklength = sizeof(server.client_addr);

	if(socketType == TCP)
		ecadd_pfd(&server.pfdList, server.serverfd);
	
	return 1;
}

void ecCloseServer(void)
{	
	if(server.socketType == TCP)
	{
		for(int i = server.pfdList.length-1; i >= 0; --i)
		{
			close(server.pfdList.pfds[i].fd);
			ecdel_pfd(&server.pfdList, i);
		}
	
		free(server.pfdList.pfds);
	}else if(server.socketType == UDP)
	{
		for(int i = server.addrList.length; i >= 0; --i)
			ecdel_addr(&server.addrList, i);
		
		free(server.addrList.addrs);
	}
}

int ecPollUDP(void)
{
	struct sockaddr_storage client_addr;
	
	socklen_t len = sizeof(struct sockaddr_storage);
	int nbytes = recvfrom(server.serverfd, server.data, server.dataLength, 0, (struct sockaddr*)&client_addr, &len);
	
	if(nbytes == -1)
	{
		AppendToLog(ERR_FAULTY_DATA);
		return 0;
	}

	printf("%d\n", nbytes);

	int found = 0;
	int i = 0;
	for(i = 0; i < server.addrList.length; ++i)
	{
		if(server.addrList.addrs[i].ss_family == client_addr.ss_family)
		{
			found = 1;
			break;
		}
	}
	
	if(!found)
	{
		ecadd_addr(&server.addrList, client_addr);
		server.AcceptedClientCallback(server.addrList.length-1);
	}
	
	if(server.DataReceivedCallback != 0)
		server.DataReceivedCallback(i, server.data);
	
	return 1;
}

int ecPollTCP(void)
{
	int num_polls = poll(server.pfdList.pfds, server.pfdList.length, 0);
	
	if(num_polls == -1)
	{
		AppendToLog(ERR_POLL);
		return 0;
	}
	
	for(int i = 0; i < server.pfdList.length; ++i)
	{
		struct pollfd fd = server.pfdList.pfds[i];

		if(!(fd.revents & POLLIN))
			continue;

		if(fd.fd == server.serverfd)
		{
			server.newfd = accept(server.serverfd, (struct sockaddr*)&server.client_addr, &server.socklength);

			if(server.newfd == -1)
			{
				AppendToLog(ERR_NO_OPEN_SOCK);
				return 0;
			}

			ecadd_pfd(&server.pfdList, server.newfd);
			if(server.AcceptedClientCallback != 0)
				server.AcceptedClientCallback(server.pfdList.length-2);

			continue;
		}

		int nbytes = recv(fd.fd, server.data, server.dataLength, 0);

		if(nbytes <= 0)
		{
			if(nbytes == 0)
			{
				if(server.ClosedConnectionCallback != 0)
					server.ClosedConnectionCallback(i-1);
				close(fd.fd);
				ecdel_pfd(&server.pfdList, i);
				continue;
			}

			AppendToLog(ERR_FAULTY_DATA);
			continue;
		}

		if(server.DataReceivedCallback != 0)
			server.DataReceivedCallback(i-1, server.data);
	}
	
	return 1;
}

int ecServerPollEvents(void)
{
	if(server.socketType == TCP)
		return ecPollTCP();
	else if(server.socketType == UDP)
		return ecPollUDP();
	else
	{
		AppendToLog(ERR_NO_PROTOCOL);
		return 0;
	}
}

int ecKickClient(int index)
{

	if(server.socketType == TCP)
	{
		if(index < 0 || index >= server.pfdList.length-1)
			return 0;
				
		close(server.pfdList.pfds[index+1].fd);
		return ecdel_pfd(&server.pfdList, index);
	}else if(server.socketType == UDP)
	{
		if(index < 0 || index >= server.addrList.length)
			return 0;

		return ecdel_addr(&server.addrList, index);
	}

	return 0;
}

int ecUnicast(int index, void* data)
{
	if(server.socketType == TCP && (index < 0 || index >= server.pfdList.length-1))
		return 0;

	if(server.socketType == UDP && (index < 0 || index >= server.addrList.length))
		return 0;
	
	int nbytes = 0;
	if(server.socketType == TCP)
		nbytes = send(server.pfdList.pfds[index+1].fd, data, server.dataLength, 0);
	else if(server.socketType == UDP)
		nbytes = sendto(server.serverfd, data, server.dataLength, 0, (struct sockaddr*)&server.addrList.addrs[index], sizeof(struct sockaddr));
	else
	{
		AppendToLog(ERR_NO_PROTOCOL);
		return 0;
	}

	if(nbytes < server.dataLength)
	{
		if(nbytes == -1)
		{
			AppendToLog(ERR_CANT_SEND);
			return 0;
		}
	
		AppendToLog(ERR_PACKAGE_SEND);
		return 0;
	}
	
	return 1;
}

int* ecBroadcast(void* data)
{
	int* result, length;
	
	if(server.socketType == TCP)
		length = server.pfdList.length-1;
	else if(server.socketType == UDP)
		length = server.addrList.length-1;

	result = (int*)malloc(length*sizeof(int));

	for(int i = 0; i < length; ++i)
	{
		if(!ecUnicast(i, data))
		{
			result[i] = 0;
			AppendToLog(ERR_PACKAGE_SEND);
			continue;
		}
		result[i] = 1;
	}
	
	return result;
}

int* ecMulticast(int* clients, int num, void* data)
{
	int* result = (int*)malloc(num*sizeof(int));
	
	for(int i = 0; i < num; ++i)
	{
		if(!ecUnicast(clients[i], data))
		{
			result[i] = 0;
			AppendToLog(ERR_PACKAGE_SEND);
			continue;
		}
		result[i] = 1;
	}
	
	return result;
}

void ecServerCloseCallback(void (*func)(int))
{
	server.ClosedConnectionCallback = func;
}

void ecServerDataCallback(void (*func)(int, void*))
{
	server.DataReceivedCallback = func;
}

void ecServerClientCallback(void (*func)(int))
{
	server.AcceptedClientCallback = func;
}