#ifndef EASY_CONNECT_SERVER_H_
#define EASY_CONNECT_SERVER_H_

#include <ecCore.h>
#include <ecClient.h>
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
	int serverfd;
	int maxClientCount, clientCount;
	ECClient *clients;
	struct sockaddr_in server_addr;
}ECServer;

typedef void (*ECSERVERDATARECEIVEPROC)(ECClient *client, char *ip, int port, int nsize, void *data);
typedef void (*ECSERVERCONNECTIONCREATEPROC)(char *ip, int port);
typedef void (*ECSERVERCONNECTIONTERMINATEDPROC)(char *ip, int port);

unsigned int ECServer_Start(ECServer *server, ecConfig *config, ECenum connectionType, int port, int maxClients);
unsigned int ECServer_Send(ECServer *server, ecConfig *config, ECClient *client, char *ip, int fd, int nsize, void *data);
unsigned int ECServer_Kick(ECServer *server, ECClient *client, char *ip, int fd);
unsigned int ECServer_Shutdown(ECServer *server);

ECSERVERDATARECEIVEPROC             ECServer_OnDataReceive(ECSERVERDATARECEIVEPROC newHandler);
ECSERVERCONNECTIONCREATEPROC        ECServer_OnConnectionCreate(ECSERVERCONNECTIONCREATEPROC newHandler);
ECSERVERCONNECTIONTERMINATEDPROC    ECServer_OnConnectionTerminated(ECSERVERCONNECTIONTERMINATEDPROC newHandler);

#endif