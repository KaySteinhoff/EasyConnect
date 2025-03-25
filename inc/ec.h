#ifndef EASY_CONNECT_H_
#define EASY_CONNECT_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef enum
{
	TCP = SOCK_STREAM,
	UDP = SOCK_DGRAM
}ECenum;

typedef struct
{
	int clientfd;
	struct sockaddr_in inet_addr;
}ECClient;

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
typedef void (*ECCLIENTDATARECEIVEPROC)(int nsize, void *data);
typedef void (*ECCLIENTCONNECTIONTERMINATEDPROC)(void);

char* ECErrorCodeToString(unsigned int errorCode);

unsigned int SetReceiveBuffer(void *ptr, unsigned int size);

unsigned int InitEC(int argc, char **argv);
unsigned int ECServer_Start(ECServer *server, ECenum connectionType, int port, int maxClients);
unsigned int ECClient_Connect(ECClient *client, ECenum connectionType, char *ip, int port);

unsigned int ECServer_Send(ECServer *server, ECClient *client, char *ip, int fd, int nsize, void *data);
unsigned int ECClient_Send(ECClient *client, int nsize, void *data);

unsigned int ECServer_Kick(ECServer *server, ECClient *client, char *ip, int fd);
unsigned int ECClient_Disconnect(ECClient *client);
unsigned int ECServer_Shutdown(ECServer *server);

ECSERVERDATARECEIVEPROC 		ECServer_OnDataReceive(ECSERVERDATARECEIVEPROC newHandler);
ECSERVERCONNECTIONCREATEPROC 		ECServer_OnConnectionCreate(ECSERVERCONNECTIONCREATEPROC newHandler);
ECSERVERCONNECTIONTERMINATEDPROC 	ECServer_OnConnectionTerminated(ECSERVERCONNECTIONTERMINATEDPROC newHandler);
ECCLIENTDATARECEIVEPROC 		ECClient_OnDataReceive(ECCLIENTDATARECEIVEPROC newHandler);
ECCLIENTCONNECTIONTERMINATEDPROC 	ECClient_OnConnectionTerminated(ECCLIENTCONNECTIONTERMINATEDPROC newHandler);

#endif
