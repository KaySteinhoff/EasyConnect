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

ECCLIENTDATARECEIVEPROC clientDataProc = NULL;
ECCLIENTCONNECTIONTERMINATEDPROC clientConnTermProc = NULL;

extern void zero(void *ptr, unsigned int size);
extern void SetFdToNonBlocking(int fd);
extern unsigned int ipToStr(in_addr_t ip, char *str);
extern unsigned int strToIP(in_addr_t *addr, char *ip);

extern unsigned int ecInitialized;

extern unsigned int UDP_ReceiveBufferLength;
extern void *UDP_ReceiveBuffer;

void ClientTCP_Process(ECClient *client)
{
	while(1)
	{
		int pkgLen = -1;
		if((pkgLen = recvfrom(client->clientfd, UDP_ReceiveBuffer, UDP_ReceiveBufferLength, 0, NULL, 0)) < 0)
			continue;
		// Read data and, if provided, pass to the given receive callback
		if(clientDataProc)
			clientDataProc(pkgLen, UDP_ReceiveBuffer);
	}
}

void ClientUDP_Process(ECClient *client)
{
	while(1)
	{
		int pkgLen = -1;
		if((pkgLen = recvfrom(client->clientfd, UDP_ReceiveBuffer, UDP_ReceiveBufferLength, 0, NULL, 0)) < 0)
			continue;

		// Read data and, if provided, pass to the given receive callback
		if(clientDataProc)
			clientDataProc(pkgLen, UDP_ReceiveBuffer);
	}
}

void* clientProcess(void *ptr)
{
	ECClient *client = ptr;

	int length = sizeof(int), type = 0;
	getsockopt(client->clientfd, SOL_SOCKET, SO_TYPE, &type, (socklen_t*)&length);
	if(type == TCP)
		ClientTCP_Process(client);
	else if(type == UDP)
		ClientUDP_Process(client);

	pthread_exit(0);
}

unsigned int ECClient_Connect(ECClient *client, ECenum connectionType, char *ip, int port)
{
	if(!client)
		return ERR_NULL_REFERENCE;

	if(!ecInitialized)
		return ERR_UNINITIALIZED;

	unsigned int err = 0;
	in_addr_t addr = 0;
	if((err = strToIP(&addr, ip)))
		return err;

	zero(&client->inet_addr, sizeof(client->inet_addr));
	client->inet_addr.sin_family = AF_INET;
	client->inet_addr.sin_addr.s_addr = htonl(addr);
	client->inet_addr.sin_port = htons(port);

	if((client->clientfd = socket(AF_INET, connectionType, 0)) < 0)
		return ERR_NETWORK;

	if(connectionType == TCP && connect(client->clientfd, (struct sockaddr*)&client->inet_addr, sizeof(client->inet_addr)) < 0)
		return ERR_NETWORK;

	// Start receiving thread
	pthread_t thread;
	pthread_create(&thread, 0, clientProcess, client);
	pthread_detach(thread);

	return ERR_SUCCESS;
}

unsigned int ECClient_Send(ECClient *client, int nsize, void *data)
{
	if(!client || !data)
		return ERR_NULL_REFERENCE;
	if(!ecInitialized)
		return ERR_UNINITIALIZED;

	int length = sizeof(int), type = 0;
	getsockopt(client->clientfd, SOL_SOCKET, SO_TYPE, &type, (socklen_t*)&length);
	if(type == TCP)
	{
		write(client->clientfd, &nsize, 4);
		write(client->clientfd, data, nsize);
	}
	else if(type == UDP && sendto(client->clientfd, data, nsize, 0, (const struct sockaddr*)&client->inet_addr, sizeof(client->inet_addr)) != nsize)
		return ERR_NETWORK;

	return ERR_SUCCESS;
}

unsigned int ECClient_Disconnect(ECClient *client)
{
	if(!client)
		return ERR_NULL_REFERENCE;

	if(!ecInitialized)
		return ERR_UNINITIALIZED;

	close(client->clientfd);
	return ERR_SUCCESS;
}

ECCLIENTDATARECEIVEPROC ECClient_OnDataReceive(ECCLIENTDATARECEIVEPROC newHandler)
{
	void *tmp = clientDataProc;
	clientDataProc = newHandler;
	return tmp;
}

ECCLIENTCONNECTIONTERMINATEDPROC ECClient_OnConnectionTerminated(ECCLIENTCONNECTIONTERMINATEDPROC newHandler)
{
	void *tmp = clientConnTermProc;
	clientConnTermProc = newHandler;
	return tmp;
}
