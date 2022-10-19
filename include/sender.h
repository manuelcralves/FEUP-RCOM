#ifndef _SENDER_H
#define _SENDER_H

#include <unistd.h>
#include <signal.h>
#include "./utils.h"

unsigned char* stuffing(const unsigned char *buf, int *bufSize);

void stateMachineSender(unsigned char byteReceived, enum state * currentState, unsigned char * controlField);

int writeInPort(int fd,const void* buf, size_t n);

int sendFrame(unsigned char* frame, int frameSize);
#endif