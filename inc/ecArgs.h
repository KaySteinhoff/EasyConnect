#ifndef EASY_CONNECT_ARGS_H_
#define EASY_CONNECT_ARGS_H_

typedef struct {
    int ipv;
    unsigned int receiveBufferSize;
    void *receiveBuffer;
} ecConfig;

unsigned int ECParseArgs(ecConfig *config, int argc, char **argv);

#endif