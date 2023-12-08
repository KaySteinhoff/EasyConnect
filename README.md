# EasyConnect | A C networking library for dummies

EasyConnect was made with the intention of making networking in C easier.


When using gcc the -I and -L tags are needed.
```
gcc file.c -I headers -L bin/lib -lEasyConnect -o program
```

__*NOTE*__

When using UDP it is important to note that calling ecDisconnect on the client won't work as it is connectionless.
A packet containing (your)specialized data will need to be send to the server and detected in the DataReceivedCallback()
to manually remove the client from the list of known sockets.

### Table of contents

- EasyConnect callbacks
- EasyConnect functions
- EasyServer
	- EasyServer callback documentation
	- EasyServer function documentation
- EasyClient
	- EasyClient callback documentation
	- EasyClient function documentation
- Known issues

## EasyConnect callbacks

### EasyErrorCallback
This callback is invoked after an error message was appended to the error log.
The passed char* is the error message that was appended. To get the log refer to GetError() in the documentation.
```C
//Example callback
void OnError(char* errorMessage);
```

## EasyConnect functions

### GetError
This function reads from the error log and returns a char* of errors.
Returns "No Errors" if no error messages are found.
```C
void GetError(void);
```

Example:
```C
#include <EasyConnect.h>

int main()
{	
	if(!ecCreateServer("127.0.0.1", 5000, TCP, 10, 64))
	{
		printf("%s\n", GetError());
		return -1;
	}
	
	return 0;
}
```

### SetErrorCallback
This function sets the ErrorCallback to the passed function pointer.
```C
void SetErrorCallback(void (*func)(char*));
```

Example:
```C
#include <EasyConnect.h>

void Error(char* errormsg)
{
	printf("%s\n", errormsg);
	exit(-1);
}

int main()
{
	SetErrorCallback(Error);

	ecCreateServer("127.0.0.1", "5000", TCP, 10, 64);

	return 0;	
}
```


# EasyServer

## EasyServer callback documentation

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

## EasyServer function documentation

### ecCreateServer
This function returns a non-zero integer if the server was successfully created.
```C
int ecCreateServer(char* openaddress, uint32_t port, int socketType, int maxClients, int dataLength);
```

Example:
```C
#include <EasyConnect.h>

int main()
{
	ecCreateServer("127.0.0.1", 5000, TCP, 10, 64);
	return 0;
}
```

|Parameter|Usage|
|---|---|
|char* openaddress|The address the server has to use|
|uint32_t port|The port number to start the server on as a char pointer|
|int socketType|The socketType to use. Can be either TCP or UDP depending on how to handle the server and client sockets|
|int maxClients|The maximum number of clients on the server|
|int dataLength|The length, in bytes, of the packets send between server and client|

### ecCloseServer
This function stops the server and kicks all clients currently connected to it.
```C
void ecCloseServer(void);
```

Example:
```C
#include <EasyConnect.h>

void closed(int index)
{
	if(index == 0)
		ecCloseServer();
}

int main()
{
	ecCreateServer("127.0.0.1", 5000, TCP, 10, 64);
	
	ecServerCloseCallback(closed);
	
	while(1)
	{
		ecServerPollEvents();
	}
	
	return 0;
}
```

|Parameters|Usage|
|---|---|

### ecServerPollEvents
This function checks if clients joined/send a package/left, invoking the corresponding callback if found.
```C
int ecServerPollEvents(void);
```

Example:
```C
#include <EasyConnect.h>

void joined(int index)
{
	char text[64] = "Hello from Server";
	
	if(!ecUnicast(index, text))
		printf("Message send!\n");
	else
		printf("Failed to send message!\n");
	
	ecKickClient(index);
}

int main()
{
	ecCreateServer("127.0.0.1", 5000, TCP, 10, 64);
	
	ecServerClientCallback(joined);
	
	while(1)
	{
		ecServerPollEvents();
	}
	
	return 0;
}
```

### ecServerCloseCallback
This function sets the ClosedConnectionCallback callback of the server.
```C
void ecServerCloseCallback(void (*func)(int));
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
	ecCreateServer("127.0.0.1", 5000, TCP, 10, 64);
	
	ecServerCloseCallback(closed);

	return 0;
}
```

|Parameters|Usage|
|---|---|
|void (\*func)(int)|The function pointer to the function invoked by the callback|

### ecServerDataCallback
This function sets the DataReceivedCallback callback of the server.
```C
void ecServerDataCallback(void (*func)(int, void*));
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
	ecCreateServer("127.0.0.1", 5000, TCP, 10, 64);
	
	ecServerDataCallback(receive);

	return 0;
}
```

|Parameters|Usage|
|---|---|
|void (\*func)(int, void*)|The function pointer to the function invoked by the callback|

### ecServerClientCallback
This function sets the AcceptedClientCallback callback of the server.
```C
void ecServerClientCallback(void (*func)(int));
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
	ecCreateServer("127.0.0.1", 5000, TCP, 10, 64);
	
	ecServerClientCallback(joined);

	return 0;
}
```

|Parameters|Usage|
|---|---|
|void (\*func)(int)|The function pointer to the function invoked by the callback|

### ecKickClient
This function kicks a client currently connected to the server.
```C
void ecKickClient(int index);
```

Example:
```C
#include <EasyClient.h>

void joined(int index)
{
	char text[64] = "Hello from Server!";
	
	ecUnicast(index, text);
	
	ecKickClient(index);
}

int main()
{
	ecCreateServer("127.0.0.1", 5000, TCP, 10, 64);
	
	ecServerClientCallback(joined);
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|int index|The index of the socket on the server|

### ecUnicast
This function sends a package to a single client and returns a non-zero value if successful.
```C
int ecUnicast(int index, void* data);
```

Example:
```C
#include <EasyClient.h>

void joined(int index)
{
	char text[64] = "Hello from Server!";
	
	ecUnicast(index, text);
	
	ecKickClient(index);
}

int main()
{
	ecCreateServer("127.0.0.1", 5000, TCP, 10, 64);
	
	ecServerClientCallback(joined);
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|int index|The index of the socket to send to|
|void* data|The package to send|

### ecBroadcast
This function send a package to all clients connected to the server.
It returns an int* equal in length to the number of connected clients either 0(unsuccessful) or 1(successful).
```C
int* ecBroadcast(void* data);
```

Example:
```C
#include <EasyConnect.h>

void joined(int index)
{
	char text[64];
	
	//I know it's not safe but idc
	sprintf(text, "Client joined on socket %d", index);
	
	ecBroadcast(text);
}

int main()
{	
	ecCreateServer("127.0.0.1", 5000, TCP, 10, 64);
	
	ecServerClientCallback(joined);

	return 0;
}
```

|Parameters|Usage|
|---|---|
|void* data|The package to send|

### ecMulticast
This function sends a package to some clients although it can be used to send it to all or one(why tho?).
It also returns an int* equal in length to the input int*, the values either being 0(unsuccessful) or 1(successful).
```C
int* ecMulticast(int* clients, int num, void* data);;
```

Example:
```C
#include <EasyConnect.h>

int numClients = 0;

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
	
	ecMulticast(recp, numClients-1, data);
}

int main()
{	
	ecCreateServer("127.0.0.1", 5000, TCP, 10, 64);
	
	ecServerClientCallback(joined);
	ecServerCloseCallback(closed);
	ecServerDataCallback(received);
		
	return 0;
}
```

|Parameters|Usage|
|---|---|
|int* clients|The int* containing the indices of the sockets to send to|
|int num|The number of receipients|
|void* data|The package to send|

# EasyClient

## EasyClient callback documentation

### ConnectionClosedCallback
This callback is called when the connection is closed from the server side.
When this callback is invoked ecClientPollEvents() will return 0;
```C
//Example callback
void ClosedConnection(void);
```

### DataReceivedCallback
This callback is called when a package is received from the server.
```C
//Example callback
void DataReceived(void* data);
```

### EasyClient function documentation
### ecCreateClient
This function returns a non-zero value if te client was successfully created.
```C
int ecCreateClient(char* hostaddress, uint32_t port, int socketType, int dataLength);
```

Example:
```C
#include <EasyConnect.h>

int main()
{
	ecCreateClient("127.0.0.1", 5000, TCP, 64);
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|char* hostaddress|The IP to connect to|
|uint32_t port|The port to connect through as a 32 bit unsigned integer|
|int socketType|Can be either TCP or UDP. Will handle the socket accordingly|
|int dataLength|The length of the packets send between server and client in bytes|

### ecConnectClient
This function connects to the server IP set in ecCreateClient() and returns a non-zero value if connected successfully.
```C
int ecConnectClient();
```

Example:
```C
#include <EasyConnect.h>

int main()
{
	ecCreateClient("127.0.0.1", 5000, TCP, 64);
		
	if(!ecConnectClient())
		printf("Failed to connect!\n");
	
	return 0;
}
```

|Parameters|Usage|
|---|---|

### ecDisconnect
This function disconnects the client.
```C
void ecDisconnect(void);
```

Example:
```C
#include <EasyConnect.h>

int main()
{
	ecCreateClient("127.0.0.1", 5000, TCP, 64);
		
	if(ecConnectClient())
		ecDisconnect();
	
	return 0;
}
```

|Parameters|Usage|
|---|---|

### ecSend
This function sends a package to the server the client is connected to.
It will return a non-zero value if the package was successfully send.
```C
int ecSend(void* data);
```

Example:
```C
#include <EasyConnect.h>

int main()
{	
	ecCreateClient("127.0.0.1", 5000, TCP, 64);
	
	ecClientUpdate(update);
	
	if(!ecConnectClient())
	{
		return -1;
	}
	
	char text[64] = "Hello from client!\n";

	ecSend(text);
	ecDisconnect();
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|void* data|The data to be send|

### ecClientClosedCallback
This function sets the ConnectionClosedCallback callback to the passed function pointer.
```C
void ecClientClosedCallback(void (*func)());
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
	ecCreateClient("127.0.0.1", 5000, TCP, 64);
	
	ecClientClosedCallback(closed);
	
	ecConnectClient();
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|void (\*func)()|The function pointer to the function that is to be called|

### ecClientDataCallback
This function sets the DataReceivedCallback callback to the passed function pointer.
```C
void ecClientDataCallback(void (*func)(void*));
```

Example:
```C
#include <EasyConnect.h>

char* input;

void receive(void* data)
{
	printf("%s\n", (char*)data);
}

int main()
{	
	ecCreateClient("127.0.0.1", 5000, TCP, 64);
	
	ecClientDataCallback(receive);
	
	ecConnectClient();
	
	return 0;
}
```

|Parameters|Usage|
|---|---|
|void (\*func)(void*)|The function pointer to the function that is to be called|

## Known Issues

- poll.h is slow when managing large amounts of fds
- No cross-platform support only linux
- Detecting if a client is known happens with a speed of O(n), going to add a hashtable
- Only TCP support, UDP still has to be implemented