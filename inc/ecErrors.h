#ifndef EASY_CONNECT_ERRORS_H_
#define EASY_CONNECT_ERRORS_H_

extern unsigned int EC_ERR_SUCCESS;
extern unsigned int EC_ERR_UNINITIALIZED;
extern unsigned int EC_ERR_NULL_REFERENCE;
extern unsigned int EC_ERR_INVALID_ARGUMENT;
extern unsigned int EC_ERR_INVALID_OPERATION;
extern unsigned int EC_ERR_NETWORK;
extern unsigned int EC_ERR_INDEX_OUT_OF_RANGE;

unsigned int ECErrorCodeToString(unsigned int errorCode, const char **ptr);

#endif