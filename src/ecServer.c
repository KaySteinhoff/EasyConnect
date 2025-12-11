#include <ec.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __unix__
	#include <unistd.h>
	#include <sys/fcntl.h>
	#include <pthread.h>
#elif defined(__WIN32)
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <process.h>
	#define close closesocket
	#define pthread_t HANDLE
	#define pthread_create(thr, attr, func, arg) *(thr) = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)(func), arg, 0, NULL)
	#define pthread_detach(thr) CloseHandle(thr)
	#define pthread_exit(code) _endthreadex(code)
#endif

static ECSERVERDATARECEIVEPROC serverDataProc = NULL;
static ECSERVERCONNECTIONCREATEPROC serverConnCreateProc = NULL;
static ECSERVERCONNECTIONTERMINATEDPROC serverConnTermProc = NULL;

extern unsigned int ipToStr(in_addr_t ip, char *str);
extern unsigned int strToIP(in_addr_t *addr, char *ip);

static void ECServerReadData(ECServer *server, int clientIdx, unsigned char *initial, unsigned char **data, int *pkgLen)
{
	*data = malloc(1024);
	if(!*data)
		return;

	memcpy(*data, initial, *pkgLen);
	int pkgCapacity = 1024, prevBytesRead;
	while((*pkgLen = read(server->clients[clientIdx].clientfd, *data + (pkgCapacity << 1), (pkgCapacity << 1))) > 0)
	{
		unsigned char *tmp = realloc(*data, pkgCapacity << 1);
		if(!tmp)
		{
			free(*data);
			*data = NULL;
			return;
		}

		*data = tmp;
		pkgCapacity <<= 1;
		prevBytesRead = *pkgLen;
	}

	*pkgLen = (pkgCapacity << 1) + prevBytesRead;
}

static void ECServerRelayData(ECServer *server, int clientIdx, void *data, int pkgLen)
{
	if(!serverDataProc || !data)
		return;

	char ip[16] = { 0 };
	ipToStr(ntohl(server->clients[clientIdx].inet_addr.sin_addr.s_addr), ip);
	serverDataProc(server, &server->clients[clientIdx], ip, ntohs(server->clients[clientIdx].inet_addr.sin_port), pkgLen, data);
}

static void ECServerHandleDisconnect(ECServer *server, int clientIdx)
{
	if(!serverConnTermProc)
	{
		close(server->clients[clientIdx].clientfd);
		server->clients[clientIdx] = server->clients[--server->clientCount];
		return;
	}

	char ip[16] = { 0 };
	ipToStr(ntohl(server->clients[clientIdx].inet_addr.sin_addr.s_addr), ip);
	serverConnTermProc(server, ip, ntohs(server->clients[clientIdx].inet_addr.sin_port));
}

static void ECServerHandlePackage(ECServer *server, int clientIdx)
{
	// first attempt using stack a allocated buffer, we'll resize if needed
	int pkgLen = 512;
	unsigned char dummy[512] = { 0 };
	if((pkgLen = read(server->clients[clientIdx].clientfd, &dummy, 512)) < 0)
		return;

	switch(pkgLen)
	{
		case 0: // disconnect
			ECServerHandleDisconnect(server, clientIdx);
			break;
		case 512: // pkgLen is 512 and since that's the maximum we can read in one go there might be more and we need to keep reading
			unsigned char *data = NULL;
			ECServerReadData(server, clientIdx, dummy, &data, &pkgLen);
			ECServerRelayData(server, clientIdx, data, pkgLen);
			if(data)
				free(data);
			break;
		default: // pkgLen is between 0 and 512, meaning we've read the entire package
			ECServerRelayData(server, clientIdx, dummy, pkgLen);
			break;
	}
}

static void ECServerCreateConnection(ECServer *server, int clientIdx)
{
	if(!serverConnCreateProc)
		return;

	char ip[16] = { 0 };
	ipToStr(ntohl(server->clients[clientIdx].inet_addr.sin_addr.s_addr), ip);
	serverConnCreateProc(server, ip, ntohs(server->clients[clientIdx].inet_addr.sin_port));
}

static void ServerTCP_Process(ECServer *server)
{
	int fdFlags = fcntl(server->serverfd, F_GETFL, 0);
	fcntl(server->serverfd, F_SETFL, fdFlags | O_NONBLOCK);

	struct sockaddr_in client_addr = { 0 };
	socklen_t clientlen = sizeof(client_addr);

	int connectedfd = 0;
	ECClient clients[server->maxClientCount];
	server->clients = clients;

	listen(server->serverfd, server->maxClientCount);
	while(1)
	{
		// Check if any connection request was made
		while((connectedfd = accept(server->serverfd, (struct sockaddr*)&client_addr, &clientlen)) > 0)
		{
			clients[server->clientCount].clientfd = connectedfd;
			clients[server->clientCount++].inet_addr = client_addr;
			ECServerCreateConnection(server, server->clientCount - 1);
		}

		// Poll received packages
		for(int i = 0; i < server->clientCount; ++i)
			ECServerHandlePackage(server, i);
	}
}

static void ServerUDP_Process(ECServer *server)
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
		if((pkgLen = recvfrom(server->serverfd, server->config->receiveBuffer, server->config->receiveBufferSize, 0, (struct sockaddr*)&client.inet_addr, &clientlen)) < 0)
			continue;

		if(serverDataProc)
		{
			ipToStr(ntohl(client.inet_addr.sin_addr.s_addr), ip);
			serverDataProc(server, &client, ip, ntohs(client.inet_addr.sin_port), pkgLen, server->config->receiveBuffer);
		}
		memset(server->config->receiveBuffer, 0, pkgLen);
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

unsigned int ECServer_Start(ECServer *server, ecConfig *config, ECenum connectionType, int port, int maxClients)
{
	if(!server || !config)
		return EC_ERR_NULL_REFERENCE;

	if((config->ipv != 4 && config->ipv != 6) || maxClients <= 0 || port <= 0)
		return EC_ERR_INVALID_ARGUMENT;

	memset(&server->server_addr, 0, sizeof(server->server_addr));
	server->server_addr.sin_family = config->ipv == 4 ? AF_INET : AF_INET6;
	server->server_addr.sin_addr.s_addr = INADDR_ANY;
	server->server_addr.sin_port = htons(port);
	server->maxClientCount = maxClients;

	if(bind((server->serverfd = socket(server->server_addr.sin_family, connectionType, 0)), (struct sockaddr*)&server->server_addr, sizeof(server->server_addr)) < 0)
		return EC_ERR_NETWORK;

	// Start connecting and receiving thread
	server->config = config;
	pthread_create(&server->processingThread, 0, serverProcess, server);
	pthread_detach(server->processingThread);

	return EC_ERR_SUCCESS;
}

unsigned int ECServer_Shutdown(ECServer *server)
{
	if(!server)
		return EC_ERR_NULL_REFERENCE;

	if(server->clients)
	{
		int count = server->clientCount;
		server->clientCount = 0;
		for(int i = 0; i < count; ++i)
			close(server->clients[i].clientfd);
		server->clients = NULL;
	}

	close(server->serverfd);
	pthread_cancel(server->processingThread);
	return EC_ERR_SUCCESS;
}

static void CloseSocketByIdx(ECServer *server, int idx)
{
	close(server->clients[idx].clientfd);
	server->clients[idx] = server->clients[--server->clientCount];
}

static void SendToSocketByIdx(ECServer *server, int idx, int nsize, void *data)
{
	write(server->clients[idx].clientfd, data, nsize);
}

unsigned int ECServer_Send(ECServer *server, ECClient *client, char *ip, int fd, int nsize, void *data)
{
	if(!server || !data || !ip)
		return EC_ERR_NULL_REFERENCE;

	int length = sizeof(int), type = 0;
	getsockopt(server->serverfd, SOL_SOCKET, SO_TYPE, &type, (socklen_t*)&length);
	if(type == UDP && client)
	{
		if(sendto(server->serverfd, data, nsize, 0, (const struct sockaddr*)&client->inet_addr, sizeof(client->inet_addr)) == nsize)
			return EC_ERR_SUCCESS;
		return EC_ERR_NETWORK;
	}
	else if(type == UDP && !client)
		return EC_ERR_INVALID_OPERATION;

	if(!server->clients)
		return EC_ERR_NULL_REFERENCE;

	if(fd >= 0)
	{
		for(int i = 0; i < server->clientCount; ++i)
		{
			if(server->clients[i].clientfd != fd)
				continue;
			SendToSocketByIdx(server, i, nsize, data);
			return EC_ERR_SUCCESS;
		}
		return EC_ERR_INVALID_ARGUMENT;
	}
	else if(client)
	{
		for(int i = 0; i < server->clientCount; ++i)
		{
			if(&server->clients[i] != client)
				continue;
			SendToSocketByIdx(server, i, nsize, data);
			return EC_ERR_SUCCESS;
		}
		return EC_ERR_INVALID_ARGUMENT;
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
			return EC_ERR_SUCCESS;
		}
		return EC_ERR_INVALID_ARGUMENT;
	}
	else
		return EC_ERR_INVALID_OPERATION;

	return EC_ERR_SUCCESS;
}

unsigned int ECServer_Kick(ECServer *server, ECClient *client, char *ip, int fd)
{
	if(!server || !server->clients)
		return EC_ERR_NULL_REFERENCE;

	if(fd >= 0)
	{
		for(int i = 0; i < server->clientCount; ++i)
		{
			if(server->clients[i].clientfd != fd)
				continue;
			CloseSocketByIdx(server, i);
			return EC_ERR_SUCCESS;
		}
		return EC_ERR_INVALID_ARGUMENT;
	}
	else if(client)
	{
		for(int i = 0; i < server->clientCount; ++i)
		{
			if(&server->clients[i] != client)
				continue;
			CloseSocketByIdx(server, i);
			return EC_ERR_SUCCESS;
		}
		return EC_ERR_INVALID_ARGUMENT;
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
			return EC_ERR_SUCCESS;
		}
		return EC_ERR_INVALID_ARGUMENT;
	}
	else
		return EC_ERR_INVALID_OPERATION;

	return EC_ERR_SUCCESS;
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
