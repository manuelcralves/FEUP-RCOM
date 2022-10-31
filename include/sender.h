#ifndef _SENDER_H
#define _SENDER_H

#include <unistd.h>
#include <signal.h>
#include "./utils.h"

void alarmHandler();

int stuffing(const unsigned char *buf, int bufSize,unsigned char *newBuf);

void stateMachineSender(unsigned char byteReceived, enum state * currentState, unsigned char * controlField);

int sendDataFrame_t(unsigned char* frame, int frameSize);

int sendControlFrame_t(int fd, unsigned char adressField,unsigned char controlField, unsigned char resAdressField, unsigned char resControlField);

#endif