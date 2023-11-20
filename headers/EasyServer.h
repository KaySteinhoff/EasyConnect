#ifndef EASY_SERVER
#define EASY_SERVER

struct PFDList
{
	int size;
	int length;
	struct pollfd* pfds;
};

struct EasyServer
{
	int serverfd;
	int newfd;
	struct sockaddr_storage client_addr;
	socklen_t socklength;
	
	struct PFDList list;
	
	int dataLength;
	void* data;
	int maxClients;
	int running;
	
	void (*DataReceivedCallback)(int, void*);
	void (*AcceptedClientCallback)(int);
	void (*ClosedConnectionCallback)(int);
};

int ecadd_pfd(struct PFDList* list, int fd);
int ecdel_pfd(struct PFDList* list, int index);
int ecCreateServer(uint32_t port, int maxClients, int dataLength);
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