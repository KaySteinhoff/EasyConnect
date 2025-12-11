#ifndef EASY_CONNECT_CLIENT_H_
#define EASY_CONNECT_CLIENT_H_

#include <ecCore.h>
#include <ecArgs.h>
#ifdef __unix__
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
#elif defined(__WIN32)
	#include <winsock2.h>
	#include <ws2tcpip.h>
#endif

typedef struct
{
	int clientfd;
	struct sockaddr_in inet_addr;
}ECClient;

typedef void (*ECCLIENTDATARECEIVEPROC)(int nsize, void *data);
typedef void (*ECCLIENTCONNECTIONTERMINATEDPROC)(void);

unsigned int ECClient_Connect(ECClient *client, ecConfig *config, ECenum connectionType, char *ip, int port);
unsigned int ECClient_Send(ECClient *client, ecConfig *config, int nsize, void *data);
unsigned int ECClient_Disconnect(ECClient *client);

ECCLIENTDATARECEIVEPROC				ECClient_OnDataReceive(ECCLIENTDATARECEIVEPROC newHandler);
ECCLIENTCONNECTIONTERMINATEDPROC	ECClient_OnConnectionTerminated(ECCLIENTCONNECTIONTERMINATEDPROC newHandler);

#endif