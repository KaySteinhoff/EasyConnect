#ifndef EASY_SERVER
#define EASY_SERVER

struct PFDList
{
	int size;
	int length;
	struct pollfd* pfds;
};

struct AddrList
{
	int size;
	int length;
	struct sockaddr_storage *addrs;
};

struct EasyServer
{
	int serverfd;
	int newfd;
	int socketType;
	struct sockaddr_storage client_addr;
	socklen_t socklength;
	
	struct PFDList pfdList;
	struct AddrList addrList;
	
	int dataLength;
	void* data;
	int maxClients;
	
	void (*DataReceivedCallback)(int, void*);
	void (*AcceptedClientCallback)(int);
	void (*ClosedConnectionCallback)(int);
};

int ecCreateServer(char* openaddress, uint32_t port, int socketType, int maxClients, int dataLength);
void ecCloseServer(void);
int ecServerPollEvents(void);
void ecServerCloseCallback(void (*func)(int));
void ecServerDataCallback(void (*func)(int, void*));
void ecServerClientCallback(void (*func)(int));
int ecKickClient(int index);
int ecUnicast(int index, void* data);
int* ecBroadcast(void* data);
int* ecMulticast(int* clients, int num, void* data);

#endif