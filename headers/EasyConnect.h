#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>

#ifndef EasyConnect
#define EasyConnect

#define TCP SOCK_STREAM
#define UDP SOCK_DGRAM

#include "EasyError.h"
#include "EasyClient.h"
#include "EasyServer.h"

#endif