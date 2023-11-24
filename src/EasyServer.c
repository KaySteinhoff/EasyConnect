#include "../headers/EasyConnect.h"

struct EasyServer server;

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

int ecdel_pfd(struct PFDList* list, int index)
{	
	if(index >= list->length-1 || index < 0)
		return 0;
	
	list->pfds[index+1] = list->pfds[list->length-1];
	
	list->length--;
	
	return 1;
}

int ecCreateServer(char* openaddress, uint32_t port, int maxClients, int dataLength)
{
	int rv;
	
	struct addrinfo hints, *ai, *p;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	char str[10];
	sprintf(str, "%ld", (long)port);
	
	if((rv = getaddrinfo(openaddress, str, &hints, &ai)) != 0)
	{
		AppendToLog(ERR_SERVER_SOCK);
		return 0;
	}
		
	for(p = ai; p != NULL; p = p->ai_next)
	{
		server.serverfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		
		if(server.serverfd == -1)
			continue;
		
		int yes = 1;
		setsockopt(server.serverfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		
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
	
	if(listen(server.serverfd, maxClients) < 0)
	{
		AppendToLog(ERR_SERVER_SOCK);
		return 0;
	}
		
	server.maxClients = maxClients;
	server.running = 1;
	server.dataLength = dataLength;
	server.list.size = 1;
	server.list.length = 0;
	server.list.pfds = (struct pollfd*)malloc(sizeof(struct pollfd));
	server.data = malloc(dataLength);
	server.socklength = sizeof(server.client_addr);
		
	ecadd_pfd(&server.list, server.serverfd);
		
	return 1;
}

void ecCloseServer(void)
{	
	for(int i = 1; i < server.list.length; ++i)
	{
		close(server.list.pfds[i].fd);
		ecdel_pfd(&server.list, i-1);
	}
}

int ecServerPollEvents(void)
{
	int num_polls = poll(server.list.pfds, server.list.length, 0);
	
	if(num_polls == -1)
	{
		AppendToLog(ERR_POLL);
		return 0;
	}
	
	for(int i = 0; i < server.list.length; ++i)
	{
		struct pollfd fd = server.list.pfds[i];

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

			ecadd_pfd(&server.list, server.newfd);
			if(server.AcceptedClientCallback != 0)
				server.AcceptedClientCallback(server.list.length-2);

			continue;
		}

		int nbytes = recv(fd.fd, server.data, server.dataLength, 0);

		if(nbytes <= 0)
		{
			if(nbytes == 0)
			{
				if(server.ClosedConnectionCallback == 0)
					server.ClosedConnectionCallback(i-1);
				close(fd.fd);
				ecdel_pfd(&server.list, i);
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

int ecKickClient(int index)
{
	if(index < 0 || index >= server.list.length-1)
		return 0;
	
	close(server.list.pfds[index+1].fd);
	ecdel_pfd(&server.list, index);
	
	return 1;
}

int ecUnicast(int index, void* data)
{
	if(index < 0 || index >= server.list.length-1)
		return 0;

	if(send(server.list.pfds[index+1].fd, data, server.dataLength, 0) == -1)
	{
		AppendToLog(ERR_PACKAGE_SEND);
		return 0;
	}
	
	return 1;
}

int* ecBroadcast(void* data)
{
	int* result = (int*)malloc((server.list.length-1)*sizeof(int));

	for(int i = 0; i < server.list.length-1; ++i)
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