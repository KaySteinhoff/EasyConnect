#include <ec.h>

#include <stdlib.h>
#include <stdio.h>
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

extern void zero(void *ptr, unsigned int size);
extern void SetFdToNonBlocking(int fd);
extern unsigned int ipToStr(in_addr_t ip, char *str);
extern unsigned int strToIP(in_addr_t *addr, char *ip);

static void ServerTCP_Process(ECServer *server)
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

static void ServerUDP_Process(ECServer *server, ecConfig *config)
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
		if((pkgLen = recvfrom(server->serverfd, config->receiveBuffer, config->receiveBufferSize, 0, (struct sockaddr*)&client.inet_addr, &clientlen)) < 0)
			continue;

		if(serverDataProc)
		{
			ipToStr(ntohl(client.inet_addr.sin_addr.s_addr), ip);
			serverDataProc(&client, ip, ntohs(client.inet_addr.sin_port), pkgLen, config->receiveBuffer);
		}
		zero(config->receiveBuffer, pkgLen);
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
		return ERR_NULL_REFERENCE;

	if((config->ipv != 4 && config->ipv != 6) || maxClients <= 0 || port <= 0)
		return ERR_INVALID_ARGUMENT;

	zero(&server->server_addr, sizeof(server->server_addr));
	server->server_addr.sin_family = config->ipv == 4 ? AF_INET : AF_INET6;
	server->server_addr.sin_addr.s_addr = INADDR_ANY;
	server->server_addr.sin_port = htons(port);
	server->maxClientCount = maxClients;

	if(bind((server->serverfd = socket(server->server_addr.sin_family, connectionType, 0)), (struct sockaddr*)&server->server_addr, sizeof(server->server_addr)) < 0)
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

static void CloseSocketByIdx(ECServer *server, int idx)
{
	close(server->clients[idx].clientfd);
	server->clients[idx] = server->clients[--server->clientCount];
}

static void SendToSocketByIdx(ECServer *server, int idx, int nsize, void *data)
{
	write(server->clients[idx].clientfd, data, nsize);
}

unsigned int ECServer_Send(ECServer *server, ecConfig *config, ECClient *client, char *ip, int fd, int nsize, void *data)
{
	if(!server || !config || !data || !ip)
		return ERR_NULL_REFERENCE;

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
