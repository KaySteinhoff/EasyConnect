#include <ec.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <pthread.h>

#define ERR_SUCCESS 0
#define ERR_UNINITIALIZED 1
#define ERR_NULL_REFERENCE 2
#define ERR_INVALID_ARGUMENT 3
#define ERR_INVALID_OPERATION 4
#define ERR_NETWORK 5
#define ERR_INDEX_OUT_OF_RANGE 6

ECSERVERDATARECEIVEPROC serverDataProc = NULL;
ECSERVERCONNECTIONCREATEPROC serverConnCreateProc = NULL;
ECSERVERCONNECTIONTERMINATEDPROC serverConnTermProc = NULL;

extern void zero(void *ptr, unsigned int size);
extern void SetFdToNonBlocking(int fd);
extern unsigned int ipToStr(in_addr_t ip, char *str);
extern unsigned int strToIP(in_addr_t *addr, char *ip);

extern unsigned int ecInitialized;

extern unsigned int UDP_ReceiveBufferLength;
extern void *UDP_ReceiveBuffer;

void ServerTCP_Process(ECServer *server)
{
	SetFdToNonBlocking(server->serverfd);

	struct sockaddr_in client_addr = { 0 };
	socklen_t clientlen = sizeof(client_addr);

	int connectedfd = 0;
	ECClient clients[server->maxClientCount];
	server->clients = clients;
	char ip[16] = { 0 };

	listen(server->serverfd, server->maxClientCount);
	while(1)
	{
		// Check if any connection request was made
		while((connectedfd = accept(server->serverfd, (struct sockaddr*)&client_addr, &clientlen)) > 0)
		{
			if(serverConnCreateProc)
			{
				ipToStr(ntohl(client_addr.sin_addr.s_addr), ip);
				serverConnCreateProc(ip, ntohs(client_addr.sin_port));
			}

			clients[server->clientCount].clientfd = connectedfd;
			clients[server->clientCount++].inet_addr = client_addr;
		}

		// Poll received packages
		for(int i = 0; i < server->clientCount; ++i)
		{
			int pkgLen = -1;
			if(read(clients[i].clientfd, &pkgLen, 4) < 0)
				continue;
			if(pkgLen <= 0)
			{
				// Handle disconnect
				if(serverConnTermProc)
				{
					ipToStr(ntohl(clients[i].inet_addr.sin_addr.s_addr), ip);
					serverConnTermProc(ip, ntohs(clients[i].inet_addr.sin_port));
				}
				close(clients[i].clientfd);
				clients[i] = clients[--server->clientCount];
				continue;
			}

			// Read data and, if provided, pass to the given receive callback
			unsigned char data[pkgLen];
			read(clients[i].clientfd, data, pkgLen);
			if(serverDataProc)
			{
				ipToStr(ntohl(clients[i].inet_addr.sin_addr.s_addr), ip);
				serverDataProc(&clients[i], ip, ntohs(clients[i].inet_addr.sin_port), pkgLen, data);
			}
		}
	}
}

void ServerUDP_Process(ECServer *server)
{
	struct sockaddr_in client_addr = { 0 };
	socklen_t clientlen = sizeof(client_addr);

	char ip[16] = { 0 };
	ECClient client = {
		.clientfd = -1,
		.inet_addr = { 0 }
	};

	while(1)
	{
		int pkgLen = -1;
		if((pkgLen = recvfrom(server->serverfd, UDP_ReceiveBuffer, UDP_ReceiveBufferLength, 0, (struct sockaddr*)&client.inet_addr, &clientlen)) < 0)
			continue;

		if(serverDataProc)
		{
			ipToStr(ntohl(client.inet_addr.sin_addr.s_addr), ip);
			serverDataProc(&client, ip, ntohs(client.inet_addr.sin_port), pkgLen, UDP_ReceiveBuffer);
		}
		zero(UDP_ReceiveBuffer, pkgLen);
	}
}

void* serverProcess(void *ptr)
{
	ECServer *server = ptr;

	int length = sizeof(int), type = 0;
	getsockopt(server->serverfd, SOL_SOCKET, SO_TYPE, &type, (socklen_t*)&length );
	if(type == TCP)
		ServerTCP_Process(server);
	else if(type == UDP)
		ServerUDP_Process(server);

	pthread_exit(0);
}

unsigned int ECServer_Start(ECServer *server, ECenum connectionType, int port, int maxClients)
{
	if(!server)
		return ERR_NULL_REFERENCE;

	if(!ecInitialized)
		return ERR_UNINITIALIZED;

	zero(&server->server_addr, sizeof(server->server_addr));
	server->server_addr.sin_family = AF_INET;
	server->server_addr.sin_addr.s_addr = INADDR_ANY;
	server->server_addr.sin_port = htons(port);
	server->maxClientCount = maxClients;

	if(bind((server->serverfd = socket(AF_INET, connectionType, 0)), (struct sockaddr*)&server->server_addr, sizeof(server->server_addr)) < 0)
		return ERR_NETWORK;

	pthread_t thread;
	// Start connecting and receiving thread
	pthread_create(&thread, 0, serverProcess, server);
	pthread_detach(thread);

	return ERR_SUCCESS;
}

unsigned int ECServer_Shutdown(ECServer *server)
{
	if(!server)
		return ERR_NULL_REFERENCE;

	if(!ecInitialized)
		return ERR_UNINITIALIZED;

	if(server->clients)
	{
		int count = server->clientCount;
		server->clientCount = 0;
		for(int i = 0; i < count; ++i)
			close(server->clients[i].clientfd);
		server->clients = NULL;
	}

	close(server->serverfd);
	return ERR_SUCCESS;
}

void CloseSocketByIdx(ECServer *server, int idx)
{
	close(server->clients[idx].clientfd);
	server->clients[idx] = server->clients[--server->clientCount];
}

void SendToSocketByIdx(ECServer *server, int idx, int nsize, void *data)
{
	write(server->clients[idx].clientfd, &nsize, 4);
	write(server->clients[idx].clientfd, data, nsize);
}

unsigned int ECServer_Send(ECServer *server, ECClient *client, char *ip, int fd, int nsize, void *data)
{
	if(!server)
		return ERR_NULL_REFERENCE;
	if(!ecInitialized)
		return ERR_UNINITIALIZED;

	int length = sizeof(int), type = 0;
	getsockopt(server->serverfd, SOL_SOCKET, SO_TYPE, &type, (socklen_t*)&length);
	if(type == UDP && client)
	{
		if(sendto(server->serverfd, data, nsize, 0, (const struct sockaddr*)&client->inet_addr, sizeof(client->inet_addr)) == nsize)
			return ERR_SUCCESS;
		return ERR_NETWORK;
	}
	else if(type == UDP && !client)
		return ERR_INVALID_OPERATION;

	if(!server->clients)
		return ERR_NULL_REFERENCE;

	if(fd >= 0)
	{
		for(int i = 0; i < server->clientCount; ++i)
		{
			if(server->clients[i].clientfd != fd)
				continue;
			SendToSocketByIdx(server, i, nsize, data);
			return ERR_SUCCESS;
		}
		return ERR_INVALID_ARGUMENT;
	}
	else if(client)
	{
		for(int i = 0; i < server->clientCount; ++i)
		{
			if(&server->clients[i] != client)
				continue;
			SendToSocketByIdx(server, i, nsize, data);
			return ERR_SUCCESS;
		}
		return ERR_INVALID_ARGUMENT;
	}
	else if(ip)
	{
		in_addr_t ip_addr = 0;
		unsigned int err = 0;
		if((err = strToIP(&ip_addr, ip)))
			return err;

		ip_addr = htonl(ip_addr);
		for(int i = 0; i < server->clientCount; ++i)
		{
			if(server->clients[i].inet_addr.sin_addr.s_addr != ip_addr)
				continue;
			SendToSocketByIdx(server, i, nsize, data);
			return ERR_SUCCESS;
		}
		return ERR_INVALID_ARGUMENT;
	}
	else
		return ERR_INVALID_OPERATION;

	return ERR_SUCCESS;
}

unsigned int ECServer_Kick(ECServer *server, ECClient *client, char *ip, int fd)
{
	if(!server || !server->clients)
		return ERR_NULL_REFERENCE;
	if(!ecInitialized)
		return ERR_UNINITIALIZED;

	if(fd >= 0)
	{
		for(int i = 0; i < server->clientCount; ++i)
		{
			if(server->clients[i].clientfd != fd)
				continue;
			CloseSocketByIdx(server, i);
			return ERR_SUCCESS;
		}
		return ERR_INVALID_ARGUMENT;
	}
	else if(client)
	{
		for(int i = 0; i < server->clientCount; ++i)
		{
			if(&server->clients[i] != client)
				continue;
			CloseSocketByIdx(server, i);
			return ERR_SUCCESS;
		}
		return ERR_INVALID_ARGUMENT;
	}
	else if(ip)
	{
		in_addr_t ip_addr = 0;
		unsigned int err = 0;
		if((err = strToIP(&ip_addr, ip)))
			return err;

		ip_addr = htonl(ip_addr);
		for(int i = 0; i < server->clientCount; ++i)
		{
			if(server->clients[i].inet_addr.sin_addr.s_addr != ip_addr)
				continue;
			CloseSocketByIdx(server, i);
			return ERR_SUCCESS;
		}
		return ERR_INVALID_ARGUMENT;
	}
	else
		return ERR_INVALID_OPERATION;

	return ERR_SUCCESS;
}

ECSERVERDATARECEIVEPROC ECServer_OnDataReceive(ECSERVERDATARECEIVEPROC newHandler)
{
	void *tmp = serverDataProc;
	serverDataProc = newHandler;
	return tmp;
}

ECSERVERCONNECTIONCREATEPROC ECServer_OnConnectionCreate(ECSERVERCONNECTIONCREATEPROC newHandler)
{
	void *tmp = serverConnCreateProc;
	serverConnCreateProc = newHandler;
	return tmp;
}

ECSERVERCONNECTIONTERMINATEDPROC ECServer_OnConnectionTerminated(ECSERVERCONNECTIONTERMINATEDPROC newHandler)
{
	void *tmp = serverConnTermProc;
	serverConnTermProc = newHandler;
	return tmp;
}
