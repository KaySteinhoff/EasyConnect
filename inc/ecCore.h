#ifndef EASY_CONNECT_CORE_H_
#define EASY_CONNECT_CORE_H_

#ifdef __unix__
	#include <sys/socket.h>
#elif defined(__WIN32)
	#include <winsock2.h>
#endif

typedef enum
{
	TCP = SOCK_STREAM,
	UDP = SOCK_DGRAM
}ECenum;

#endif