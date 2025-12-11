# EasyConnect

EasyConnect is a simple networking library that probably isn't good enough to use in every situation but good enough to be usable.<br>
EasyConnect uses a multithreaded callback system to interface with the user logic.

# Building from source

This project was compiled with gcc but will probably work with any compiler.<br>

## Linux

### Dependencies
- pthread

```sh
gcc -shared -fpic src/*.c -I inc -lpthread -Wall -Werror -o libec.so
```

# Quick start

## Initialization

Regardless of wether you want to setup a server or client you'll have to call ECParseArgs() to initialize EasyConnect.<br>
Once successfully done you can setup your networking element.

```c
#include <ec.h>
#include <stdio.h>
#include <stdlib.h>

void logErr(unsigned int code)
{
	unsigned int err = 0;
	const char *str = NULL;
	if((err = ECErrorCodeToString(code, &str)))
	{
		ECErrorCodeToString(err, &str);
		code = err;
	}
	printf("%s(Code: %d)\n", str, code);
}

void die(unsigned int code)
{
	logErr(code);
	exit(code);
}

int main(int argc, char **argv)
{
	unsigned int err = 0;

	ecConfig config = { 0 };
	if((err = ECParseArgs(&config, argc, argv)))
		die(err);

	// ...
}
```

## Network setup

After initializing EasyConnect you can set your callbacks or immediately setup the networking element you wish.
```c
#include <ec.h>
#include <stdio.h>

void logErr(unsigned int code)
{
	unsigned int err = 0;
	const char *str = NULL;
	if((err = ECErrorCodeToString(code, &str)))
	{
		ECErrorCodeToString(err, &str);
		code = err;
	}
	printf("%s(Code: %d)\n", str, code);
}

void die(unsigned int code)
{
	logErr(code);
	exit(code);
}

void DataReceived(ECClient *client, char *ip, int port, int nsize, void *data)
{
	printf("%s(Port: %d):'%.*s'\n", ip, port, nsize, data);
}

int main(int argc, char **argv)
{
	unsigned int err = 0;

	ecConfig config = { 0 };
	if((err = ECParseArgs(&config, argc, argv)))
		die(err);

	ECServer server = { 0 };
	ECClient client = { 0 };

	ECServer_OnDataReceive(DataReceive);

	if((err = ECServer_Start(&server, &config, TCP, 16380, 5)))
		die(err);

	if((err = ECClient_Connect(&client, &config, TCP, "127.0.0.1", 16380)))
		die(err);

	// Or set callback here, just set before you 
	// send/receive any data as otherwise the packages may be lost
	if((err = ECClient_Send(&client, 5, "Ping"))) // 5 because of the '\0'
		logErr(err);

	// Other stuff, calling ECClient_Disconnect(&client) and ECServer_Shutdown(&server), etc..

	return 0;
}
```

## Error handling

EasyConnect returns an error code for every provided function.<br>
To convert an error code to a string call [the previously mentioned] ECErrorCodeToString() function, passing in the error code.<br>
<br>
Error coverage(inside of the library functions) contains:
- EasyConnect being uninitialized
- Null references
- Invalid arguments
- Invalid operations
- [unspecified] Networking errors (in improvement)
- Index out of range

Should ECErrorCodeToString() be given an invalid error code, an "Index out of range" error will be returned.

# Callbacks

## Server

The EasyConnect server uses three callbacks: OnDataReceive, OnConnectionCreate and OnConnectionTerminated.

### OnDataReceive

Definition:
```c
typedef void(*ECSERVERDATARECEIVEPROC)(ECClient *client, char *ip, int port, int nsize, void *data);
```

To set this callback you need to call ECServer_OnDataReceive():
```c
ECSERVERDATARECEIVEPROC oldHandler = ECServer_OnDataReceive(newHandler);
```

### OnConnectionCreated

Definition:
```c
typedef void(*ECSERVERCONNECTIONCREATEPROC)(char *ip, int port);
```

To set this callback you need to call ECServer_OnConnectionCreate():
```c
ECSERVERCONNECTIONCREATEPROC oldHandler = ECServer_OnConnectionCreate(newHandler);
```

### OnConnectionTerminated

Definition:
```c
typedef void(*ECSERVERCONNECTIONTERMINATEDPROC)(char *ip, int port);
```

To set this callback you need to call ECServer_OnConnectionTerminated():
```c
ECSERVERCONNECTIONTERMINATEDPROC oldHandler = ECServer_OnConnectionTerminated(newHandler);
```

## Client

The EasyConnect client uses two callbacks: OnDataReceive and OnConnectionTerminated(from the server side).

### OnDataReceive

Definition:
```c
typedef void(*ECCLIENTDATARECEIVEPROC)(int nsize, void *data);
```

To set this callback you need to call ECServer_OnDataReceive():
```c
ECCLIENTDATARECEIVEPROC oldHandler = ECClient_OnDataReceive(newHandler);
```

### OnConnectionTerminated

Definition:
```c
typedef void(*ECCLIENTCONNECTIONTERMINATEDPROC)(void);
```

To set this callback you need to call ECServer_OnConnectionTerminated():
```c
ECCLIENTCONNECTIONTERMINATEDPROC oldHandler = ECClient_OnConnectionTerminated(newHandler);
```

