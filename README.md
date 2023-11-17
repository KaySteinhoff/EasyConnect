# EasyConnect | A C/C++ networking library for dummies

EasyConnect was made with the intention of making networking in C/C++ practically foolproof(since I myself also tend to create some wacky server models).
This ofcourse also leads to it being good for C networking beginners.

### Table of contents

- EasyConnect functions
- EasyServer
	- EasyServer callback documentation
	- EasyServer function documentation
- EasyClient
	- EasyClient callback documentation
	- EasyClient function documentation
- Known issues

## EasyConnect functions

### InitEasyConnect
Initializes various values like the error log.
```C
void InitEasyConnect();
```

### GetError
This function reads from the error log and returns a char* of errors.
Returns "No Errors" if no error messages are found.
```C
void GetError();
```

Example:
```C
#include <EasyConnect.h>

int main()
{
	InitEasyConnect();
	
	struct EasyServer server = ecCreateServer("5000", 10, 64);
	
	char* error = GetError();
	
	if(!strcmp(error, "No Errors"))
	{
		printf("%s\n", error);
		return -1;
	}
	
	return 0;
}
```


## EasyServer

### EasyServer callback documentation

### DataReceivedCallback
This callback is called when a client sends a packet to the server and the read bytes were not faulty(recv returning < 0).
The callback will pass the index of the client which send the packet and a void* to the received package.
```C
//Example callback
void ReceiveCallback(int index, void* data);
```

To set the callback call the ecServerDataCallback() function passing a pointer to the server struct and the function pointer.

### AcceptedClientCallback
This callback is called whenever a client joins the server.
The callback passes the index to the joined client.
```C
//Example callback
void ClientJoined(int index);
```

To set the callback call the ecServerClientCallback() function passing a pointer to the server struct and the function pointer.

### ClosedConnectionCallback
This callback is called whenever a client leaves the server but before the connection is closed and the client is removed.
The callback passes the index to the joined client.
```C
//Example callback
void ClientLeft(int index);
```

To set the callback call the ecServerCloseCallback() function passing a pointer to the server struct and the function pointer.

### Update
This callback is called every tick(specified by the ecStartServer() function) and __does not__ need to be set.
```C
//Example callback
void Update();
```

To set the callback call the ecServerUpdate() function passing a pointer to the server struct and the function pointer.

### EasyServer function documentation

### ecCreateServer
This function returns a struct EasyServer instance configured to be used in the ecStartServer() function.
```C
struct EasyServer ecCreateServer(uint32_t port, int maxClients, int dataLength);
```

Example:
```C
#include <EasyConnect.h>

int main()
{
	InitEasyConnect();
	struct EasyServer server = ecCreateServer(5000, 10, 64);
	return 0;
}
```

|Parameter|Usage|
|---|---|
|char* port|The port number to start the server on as a char pointer|
|int maxClients|The maximum number of clients on the server|
|int dataLength|The length, in bytes, of the packets send between server and client|

### ecStartServer
This function starts the server and returns a non-zero value if the server successfully ran to completion. This function runs until ecStopServer() function is called on the same server instance started by ecStartServer().
```C
int ecStartServer(struct EasyServer* server, int tickMilli);
```

Example:
```C
#include <EasyConnect.h>

int main()
{
	InitEasyConnect();
	struct EasyServer server = ecCreateServer("5000", 10, 64);
	
	if(!ecStartServer(&server, 1))
		printf("%s\n", GetError());
	
	return 0;
}
```

|Patameters|Usage|
|---|---|
|struct EasyServer* server|The server instance to run|
|int tickMilli|The ticks the server attempts to reach, in milliseconds.(It's better to set it lower than what you need)|

### ecStopServer
This function stops the server and kicks all clients currently connected to it.
```C
void ecStopServer(struct EasyServer* server);
```

Example:
```C
#include <EasyConnect.h>

struct EasyServer server;

void closed(int index)
{
	if(index == 0)
		ecStopServer(&server);
}

int main()
{
	InitEasyConnect();
	server = ecCreateServer("5000", 10, 64);
	
	ecServerCloseCallback(&server, closed);
	
	if(!Start(&server))
		printf("%s\n", GetError());
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyServer* server|The server instance to stop|

### ecServerCloseCallback
This function sets the ClosedConnectionCallback callback of the passed server struct.
```C
void ecServerCloseCallback(struct EasyServer* server, void (*func)(int));
```

Example:
```C
#include <EasyConnect.h>

void closed(int index) 
{
	printf("Client %d closed the connection!\n", index);
}

int main()
{
	InitEasyConnect();
	struct EasyServer server = ecCreateServer("5000", 10, 64);
	
	ecServerCloseCallback(&server, closed);

	ecStartServer(&server, 1);

	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyServer* server|The server struct of which the callback is to set|
|void (\*func)(int)|The function pointer to the function called by the callback|

### ecServerDataCallback
This function sets the DataReceivedCallback callback of the passed server struct.
```C
void ecServerDataCallback(struct EasyServer* server, void (*func)(int, void*));
```

Example:
```C
#include <EasyConnect.h>

void receive(int index, void* data) 
{
	printf("Client %d send message: %s\n", index, (char*)data);
}

int main()
{
	InitEasyConnect();
	struct EasyServer server = ecCreateServer("5000", 10, 64);
	
	ecServerDataCallback(&server, receive);

	ecStartServer(&server, 1);

	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyServer* server|The server struct of which the callback is to set|
|void (\*func)(int, void*)|The function pointer to the function called by the callback|

### ecServerClientCallback
This function sets the AcceptedClientCallback callback of the passed server struct.
```C
void ecServerClientCallback(struct EasyServer* server, void (*func)(int));
```

Example:
```C
#include <EasyConnect.h>

void joined(int index) 
{
	printf("New client joined on socket %d\n", index);
}

int main()
{
	InitEasyConnect();
	struct EasyServer server = ecCreateServer("5000", 10, 64);
	
	ecServerClientCallback(&server, joined);

	ecStartServer(&server, 1);

	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyServer* server|The server struct of which the callback is to set|
|void (\*func)(int)|The function pointer to the function called by the callback|

### ecServerUpdate
This function sets the Update callback of the passed server struct.
```C
void ecServerUpdate(struct EasyServer* server, void (*func)());
```

Example:
```C
#include <EasyConnect.h>

void update() 
{
	printf("Server tick\n");
}

int main()
{
	InitEasyConnect();
	struct EasyServer server = ecCreateServer("5000", 10, 64);
	
	ecServerUpdate(&server, update);

	ecStartServer(&server, 1);

	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyServer* server|The server struct of which the callback is to set|
|void (\*func)()|The function pointer to the function called by the callback|

### ecKickClient
This function kicks a client currently connected to the server.
```C
void ecKickClient(struct EasyServer* server, int index);
```

Example:
```C
#include <EasyClient.h>

struct EasyServer server;

void joined(int index)
{
	char text[64] = "Hello from Server!";
	
	ecUnicast(&server, index, text);
	
	ecKickClient(&server, index);
}

int main()
{
	server = ecCreateServer("5000", 10, 64);
	
	ecServerClientCallback(&server, joined);
	
	ecStartServer(&server);

	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyServer* server|The server the client is to be kicked from|
|int index|The index of the socket on the server|

### ecUnicast
This function sends a package to a single client and returns a non-zero value if successful.
```C
int ecUnicast(struct EasyServer* server, int index, void* data);
```

Example:
```C
#include <EasyClient.h>

struct EasyServer server;

void joined(int index)
{
	char text[64] = "Hello from Server!";
	
	ecUnicast(&server, index, text);
	
	ecKickClient(&server, index);
}

int main()
{
	server = ecCreateServer("5000", 10, 64);
	
	ecServerClientCallback(&server, joined);
	
	ecStartServer(&server);

	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyServer* server|The server the receipient is connected to|
|int index|The index of the socket to send to|
|void* data|The package to send|

### ecBroadcast
This function send a package to all clients connected to the server.
It returns an int* equal in length to the number of connected clients either 0(unsuccessful) or 1(successful).
```C
int* ecBroadcast(struct EasyServer* server, void* data);
```

Example:
```C
#include <EasyConnect.h>

struct EasyServer server;

void joined(int index)
{
	char text[64];
	
	//I know it's not safe but idc
	sprintf(text, "Client joined on socket %d", index);
	
	ecBroadcast(&server, text);
}

int main()
{
	InitEasyConnect();
	
	server = ecCreateServer("5000", 10, 64);
	
	ecServerClientCallback(&server, joined);

	ecStartServer(&server);

	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyServer* server|The server the receipients are connected to|
|void* data|The package to send|

### ecMulticast
This function sends a package to some clients although it can be used to send it to all or one(why tho?).
It also returns an int* equal in length to the input int*, the values either being 0(unsuccessful) or 1(successful).
```C
int* ecMulticast(struct EasyServer* server, int* clients, int num, void* data);;
```

Example:
```C
#include <EasyConnect.h>

int numClients = 0;
struct EasyServer server;

void joined(int index)
{
	numClients++;
}

void closed(int index)
{
	numClients--;
}

void received(int index, void* data)
{
	int* recp = (int*)malloc((numClients-1)*sizeof(int));
	
	for(int i = 0, offset = 0; i < numClients; ++i)
	{
		if(i == index)
		{
			offset = 1;
			continue;
		}
		recp[i-offset] = i;
	}
	
	ecMulticast(&server, recp, numClients-1, data);
}

int main()
{
	InitEasyServer();
	
	server = ecCreateServer("5000", 10, 64);
	
	ecServerClientCallback(&server, joined);
	ecServerCloseCallback(&server, closed);
	ecServerDataCallback(&server, received);
	
	ecStartServer(&server, 1);
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyServer* server|The server the receipients are connected to|
|int* clients|The int* containing the indices of the sockets to send to|
|int num|The number of receipients|
|void* data|The package to send|

## EasyClient

### EasyClient callback documentation

### ConnectionClosedCallback
This callback is called when the connection is closed from the server side.
Also be aware that when this callback is called the ecConnectClient() loop will end.
```C
//Example callback
void ClosedConnection();
```

### DataReceivedCallback
This callback is called when a package is received from the server.
```C
//Example callback
void DataReceived(void* data);
```

### Update
This callback is called every itteration. 
__NOTE:__
The client won't start if this callback isn't set.
```C
//Example callback
void Update();
```

### EasyClient function documentation
### ecCreateClient
This function returns a struct EasyClient instance configured to connect to a server.
```C
struct EasyClient ecCreateClient(uint32_t hostaddress, uint32_t port, int dataLength);
```

Example:
```C
#include <EasyConnect.h>

int main()
{
	InitEasyConnect();
	struct EasyClient client = ecCreateClient(INADDR_LOOPBACK, 5000, 64);
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|uint32_t hostaddress|The IP to connect to as a 32 bit unsigned integer|
|uint32_t port|The port to connect through as a 32 bit unsigned integer|
|int dataLength|The length of the packets send between server and client in bytes|

### ecConnectClient
This function connects to the server the client was configured to, starts a loop and returns a non-zero value if disconnected without issue.
```C
int ecConnectClient(struct EasyClient* client);
```

Example:
```C
#include <EasyConnect.h>

void update() {}

int main()
{
	InitEasyConnect();
	struct EasyClient client = ecCreateClient(INADDR_LOOPBACK, 5000, 64);
	
	ecClientUpdate(&client, update);
	
	ecConnectClient(&client);
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyClient* client|The client to connect|

### ecDisconnectClient
This function disconnects the passed client, ends the ecConnectClient() loop and causes it to end with a non-zero value.
```C
void ecDisconnectClient(struct EasyClient* client);
```

Example:
```C
#include <EasyConnect.h>

struct EasyClient client;

void update()
{
	ecDisconnectClient(&client);
}

int main()
{
	InitEasyConnect();
	client = ecCreateClient(INADDR_LOOPBACK, 5000, 64);
	
	ecClientUpdate(&client, update);
	
	ecConnectClient(&client);
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyClient* client|The client to disconnect|

### ecSend
This function sends a package to the server the client is connected to.
It will return a non-zero value if the package was successfully send.
```C
int ecSend(struct EasyClient client, void* data);
```

Example:
```C
#include <EasyConnect.h>

struct EasyClient client;

void update()
{
	char text[64] = "Hello from client!\n";

	ecSend(&client, text);
	ecDisconnect(&client);
}

int main()
{
	InitEasyConnect();
	
	client = ecCreateClient(INADDR_LOOPBACK, 5000, 64);
	
	ecClientUpdate(&client, update);
	
	ecConnectClient(&client);
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyClient client|The client sending the data|
|void* data|The data to be send|

### ecClientClosedCallback
This function sets the ConnectionClosedCallback callback to the passed function pointer.
```C
void ecClientClosedCallback(struct EasyClient* client, void (*func)());
```

Example:
```C
#include <EasyConnect.h>

void closed()
{
	printf("Server closed the connection\n");
}

void update() {}

int main()
{
	InitEasyConnect();
	
	struct EasyClient client = ecCreateClient(INADDR_LOOPBACK, 5000, 64);
	
	ecClientClosedCallback(&client, closed);
	ecClientUpdate(&client, update);
	
	ecConnectClient(&client);
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyClient* client|The client of which the callback is to be set|
|void (\*func)()|The function pointer to the function that is to be called|

### ecClientDataCallback
This function sets the DataReceivedCallback callback to the passed function pointer.
```C
void ecClientDataCallback(struct EasyClient* client, void (*func)(void*));
```

Example:
```C
#include <EasyConnect.h>

struct EasyClient client;
char* input;

void receive(void* data)
{
	printf("%s\n", (char*)data);
}

void update()
{
	gets(input);
	
	ecSend(&client, input);
}

int main()
{
	InitEasyConnect();
	
	client = ecCreateClient(INADDR_LOOPBACK, 5000, 64);
	
	ecClientDataCallback(&client, receive);
	ecClientUpdate(&client, update);
	
	ecConnectClient(&client);
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyClient* client|The client of which the callback is to be set|
|void (\*func)(void*)|The function pointer to the function that is to be called|

### ecClientUpdate
This function sets the update callback, which is called every itteration.
```C
void ecClientUpdate(struct EasyClient* client, void (*func)());
```

Example:
```C
#include <EasyConnect.h>

struct EasyClient client;

void update()
{
	char text[64] = "Hello from client!\n";

	ecSend(&client, text);
	ecDisconnect(&client);
}

int main()
{
	InitEasyConnect();
	
	client = ecCreateClient(INADDR_LOOPBACK, 5000, 64);
	
	ecClientUpdate(&client, update);
	
	ecConnectClient(&client);
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|struct EasyClient* client|The client of which the callback is to be set|
|void (\*func)()|The function pointer to the function that is to be called|

## Known Issues

- poll.h is slow when managing large amounts of fds
- poll() has a minimum timeout of 1 millisecond. Although I don't think I would be able to check all connections for send data in under 1 millisecond changing it(without a dependency) would be better
- EasyClient can only use IPv4
- Only TCP support UDP still has to be implemented