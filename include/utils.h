
#ifndef _UTILS_H
#define _UTILS_H

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "link_layer.h"

#define FLAG       0x7E
#define ESC        0x7D
#define STUFF(x)   x ^ (0x20)
#define BCC(x, y)  x ^ y
#define SET        0x03
#define A_RCVR     0x01
#define A_SND      0x03
#define UAKN       0x07
#define DISC       0x0B
#define RR(x)      x << 7 | 0x05
#define REJ(x)     x << 7 | 0x01
#define SEQ_NUM(x) x << 6
#define HEADER_BYTES 6
#define DATA_PKT    0x01
#define START_PKT   0x02
#define END_PKT     0x03
#define FILE_SIZE   0
#define FILE_NAME   1


enum state {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP};

void stateMachine(char byteReceived, enum state * currentState, unsigned char addressField, unsigned char controlField);

int sendControlFrame_r(int fd, unsigned char adressField, unsigned char controlField);

int receiveControlFrame_r(int fd, unsigned char adressField, unsigned char controlField);

int sendControlFrame_t(int fd, unsigned char adressField,unsigned char controlField, unsigned char resAdressField, unsigned char resControlField);
    
int isHeaderWrong(unsigned char *buf,int seqNum);

int isDuplicate(int fd,unsigned char *buf,int seqNum);

int isSeqNumWrong(unsigned char *buf,int seqNum);

int isDataBccWrong(int fd,unsigned char *buf, int bufSize,int seqNum);

int writeInPort(int fd,const void* buf, size_t n);

const char* getState (enum state s);


#endif /*_UTILS_H*/int sendInfoFrame_t(int fd, unsigned char adressField, unsigned char controlField);