#ifndef EASY_CLIENT
#define EASY_CLIENT

struct EasyClient
{
	void* data;
	int dataLength;
	int sockfd;
	struct sockaddr_in host_addr;
	
	int running;
	
	struct pollfd poll;
	
	void (*ConnectionClosedCallback)(void);
	void (*DataReceivedCallback)(void* data);
};

int ecCreateClient(char* hostaddress, uint32_t port, int dataLength);
void ecDisconnect(void);
int ecClientPollEvents(void);
int ecConnect(void);
int ecSend(void* data);
void ecClientClosedCallback(void (*func)(void));
void ecClientDataCallback(void (*func)(void*));

#endif