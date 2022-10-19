
#ifndef _UTILS_H
#define _UTILS_H

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define FLAG       0x7E
#define ESC        0x7D
#define STUFF(x)   x ^ (0x20)
#define BCC(x, y)  x ^ y
#define A_RCVR     0x01
#define A_SND      0x03
#define UAKN       0x07
#define DISC       0x0B
#define RR(x)      x << 7 | 0x05
#define REJ(x)     x << 7 | 0x01
#define SEQ_NUM(x) x << 6
#define HEADER_BYTES 6


enum state {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP};

void stateMachine(char byteReceived, enum state * currentState, unsigned char addressField, unsigned char controlField);

int sendInfoFrame(int fd, unsigned char adressField, unsigned char controlField);

int receiveInfoFrame(int fd, unsigned char adressField, unsigned char controlField);

int isHeaderWrong(unsigned char *buf);

int isDuplicate(unsigned char *buf);

int isSeqNumWrong(unsigned char *buf);

int isDataBccWrong(unsigned char *buf, int bufSize);


#endif /*_UTILS_H*/