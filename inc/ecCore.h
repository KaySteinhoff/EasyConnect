#ifndef EASY_CONNECT_CORE_H_
#define EASY_CONNECT_CORE_H_

#ifdef __unix__
	#include <netinet/in.h>
#elif defined(__WIN32)
	#include <winsock2.h>
#endif

typedef enum
{
	TCP = SOCK_STREAM,
	UDP = SOCK_DGRAM,
}ECenum;

unsigned int ipv42str(struct in_addr addr, char *ip);
unsigned int ipv62str(struct in6_addr addr, char *ip);
unsigned int str2ipv4(char *ip, struct in_addr *addr);
unsigned int str2ipv6(char *ip, struct in6_addr *addr);

#endif
