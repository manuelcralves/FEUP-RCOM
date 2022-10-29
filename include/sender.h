#ifndef _SENDER_H
#define _SENDER_H

#include <unistd.h>
#include <signal.h>
#include "./utils.h"

void alarmHandler();

int stuffing(const unsigned char *buf, int bufSize,unsigned char *newBuf);

void stateMachineSender(unsigned char byteReceived, enum state * currentState, unsigned char * controlField);

int writeInPort(int fd,const void* buf, size_t n);

int sendFrame(unsigned char* frame, int frameSize);
#endif