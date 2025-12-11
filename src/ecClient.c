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


static ECCLIENTDATARECEIVEPROC clientDataProc = NULL;
static ECCLIENTCONNECTIONTERMINATEDPROC clientConnTermProc = NULL;

extern void zero(void *ptr, unsigned int size);
extern unsigned int ipToStr(in_addr_t ip, char *str);
extern unsigned int strToIP(in_addr_t *addr, char *ip);

static void ClientTCP_Process(ECClient *client)
{
	while(1)
	{
		int pkgLen = -1;
		if((pkgLen = recvfrom(client->clientfd, client->config->receiveBuffer, client->config->receiveBufferSize, 0, NULL, 0)) < 0)
			continue;
		// Read data and, if provided, pass to the given receive callback
		if(clientDataProc)
			clientDataProc(pkgLen, client->config->receiveBuffer);
	}
}

static void ClientUDP_Process(ECClient *client)
{
	while(1)
	{
		int pkgLen = -1;
		if((pkgLen = recvfrom(client->clientfd, client->config->receiveBuffer, client->config->receiveBufferSize, 0, NULL, 0)) < 0)
			continue;

		// Read data and, if provided, pass to the given receive callback
		if(clientDataProc)
			clientDataProc(pkgLen, client->config->receiveBuffer);
	}
}

static void* clientProcess(void *ptr)
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

unsigned int ECClient_Connect(ECClient *client, ecConfig *config, ECenum connectionType, char *ip, int port)
{
	if(!client || !config || !ip)
		return EC_ERR_NULL_REFERENCE;

	unsigned int err = 0;
	in_addr_t addr = 0;
	if((err = strToIP(&addr, ip)))
		return err;

	zero(&client->inet_addr, sizeof(client->inet_addr));
	client->inet_addr.sin_family = AF_INET;
	client->inet_addr.sin_addr.s_addr = htonl(addr);
	client->inet_addr.sin_port = htons(port);

	if((client->clientfd = socket(AF_INET, connectionType, 0)) < 0)
		return EC_ERR_NETWORK;

	if(connectionType == TCP && connect(client->clientfd, (struct sockaddr*)&client->inet_addr, sizeof(client->inet_addr)) < 0)
		return EC_ERR_NETWORK;

	// Start receiving thread
	client->config = config;
	pthread_t thread;
	pthread_create(&thread, 0, clientProcess, client);
	pthread_detach(thread);

	return EC_ERR_SUCCESS;
}

unsigned int ECClient_Send(ECClient *client, int nsize, void *data)
{
	if(!client || !data)
		return EC_ERR_NULL_REFERENCE;

	int length = sizeof(int), type = 0;
	getsockopt(client->clientfd, SOL_SOCKET, SO_TYPE, &type, (socklen_t*)&length);
	if(type == TCP)
		write(client->clientfd, data, nsize);
	else if(type == UDP && sendto(client->clientfd, data, nsize, 0, (const struct sockaddr*)&client->inet_addr, sizeof(client->inet_addr)) != nsize)
		return EC_ERR_NETWORK;

	return EC_ERR_SUCCESS;
}

unsigned int ECClient_Disconnect(ECClient *client)
{
	if(!client)
		return EC_ERR_NULL_REFERENCE;

	close(client->clientfd);
	return EC_ERR_SUCCESS;
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
