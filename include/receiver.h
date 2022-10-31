#ifndef _RECEIVER_H
#define _RECEIVER_H

#include <unistd.h>
#include <signal.h>
#include "./utils.h"

int destuffing(const unsigned char* buf, int bufSize, unsigned char* packet);

void stateMachineReceiver(char byteReceived, enum state * currentState, int * prevWasFlag);

int receiveDataFrame_r(int fd,unsigned char* frame);

int sendControlFrame_r(int fd, unsigned char adressField, unsigned char controlField);

int receiveControlFrame_r(int fd, unsigned char adressField, unsigned char controlField);

#endif