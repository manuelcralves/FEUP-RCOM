#include "../include/sender.h"

#include "link_layer.h"


extern int fd;
extern LinkLayer ll;
int counter;
unsigned int flag;
unsigned int seqNum = 0;

void alarmHandler(){
	printf("alarme #%d\n", counter);
	flag = 1;
	counter++;
}

unsigned char* stuffing(const unsigned char *buf, int *bufSize) {
    unsigned char *newBuf = malloc(sizeof(char)*255);

    newBuf[0] = FLAG;
    
    int index = 1;

    for (size_t i = 1; i < (*bufSize) -1; i++){
        if (buf[i] == FLAG){
            newBuf[index] = ESC;
            index++;
            newBuf[index] = STUFF(FLAG);
            index++;
        }

        else if(buf[i] == ESC){
            newBuf[index] = ESC;
            index++;
            newBuf[index] = STUFF(ESC);
            index++;
        }
        else {
            newBuf[index] = buf[i];
            index++;
        }
    }
    
    newBuf[index] = FLAG;
    *bufSize = index+1;
    return newBuf;
}

void stateMachineSender(unsigned char byteReceived, enum state * currentState, unsigned char * controlField) {
  switch (*currentState) {
      case START:
      if(byteReceived == FLAG) *currentState = FLAG_RCV;
      break;
    case FLAG_RCV:
      if(byteReceived == A_RCVR) *currentState = A_RCV;
      else if (byteReceived != FLAG) *currentState = START;
      break;
    case A_RCV:
      if(byteReceived == FLAG) *currentState = FLAG_RCV;
      else if(byteReceived == (RR(1)) || byteReceived == (RR(0)) || byteReceived == (REJ(0)) || byteReceived == (REJ(1))) {
        *controlField = byteReceived;
        *currentState = C_RCV;
      }
      else *currentState = START;
      break;
    case C_RCV:
      if(byteReceived == FLAG) *currentState = FLAG_RCV;
      else if(byteReceived == (BCC(A_RCVR, *controlField))) *currentState = BCC_OK;
      else *currentState = START;
      break;
    case BCC_OK:
      if(byteReceived == FLAG) *currentState = STOP;
      else *currentState = START;
      break;
    default:
      break;
  }
}

int writeInPort(int fd,const void* buf, size_t n) {

    int b = 0;
    int res = 0;

    while (b != n) {
        res = write(fd,buf+b,n-b);
        
        if (res < 0) { perror("couldnt write"); return -1;}

        b+=res;
    }   

    return b;
}

int sendFrame(unsigned char* frame, int frameSize) {
    enum state currentState = START;
    char buf[255];
    int resR= 0, resW=0;
    //alarm flag
    counter = 0;
    unsigned char controlField;

    resW = writeInPort(fd,frame,frameSize);
    alarm(3);

    do {
        if (flag){
            alarm(3);
            flag = FALSE;
            resW = writeInPort(fd,frame,frameSize);
        }

        resR = read(fd,buf,1);
        if (!resR) continue;
        stateMachineSender(buf[0],&currentState,&controlField);

    } while (currentState != STOP && counter <= ll.nRetransmissions);

    alarm (0); /*write successfull*/

    int nextSeqNum;
    if (seqNum == 0) nextSeqNum = 1;
    else nextSeqNum = 0;

    if (currentState == STOP) {
        if (controlField == (REJ(seqNum))) {
            return sendFrame(frame,frameSize);
        }
        if (controlField == (RR(nextSeqNum))) {
            seqNum = nextSeqNum;
            return resW;
        }
    }

    return -1;

}
