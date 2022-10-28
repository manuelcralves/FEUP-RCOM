#ifndef _RECEIVER_H
#define _RECEIVER_H

#include <unistd.h>
#include <signal.h>
#include "./utils.h"

int destuffing(const unsigned char* buf, int bufSize, unsigned char* packet);

void stateMachineReceiver(char byteReceived, enum state * currentState, int * prevWasFlag);

int receiveFrame(int fd,unsigned char* frame);

#endif