#include "../include/sender.h"

#include "link_layer.h"


extern int fd;
extern int numTries;
int counter;
unsigned int flag = 0;
unsigned int seqNum = 0;

void alarmHandler(){
	printf("alarme #%d\n", counter);
	flag = 1;
	counter++;
}

int stuffing(const unsigned char *buf, int bufSize, unsigned char *newBuf) {

    newBuf[0] = FLAG;
    
    int index = 1;
    int aux = bufSize;

    for (size_t i = 1; i < (bufSize) -1; i++){
        if (buf[i] == FLAG){
            aux++;
            newBuf = realloc(newBuf,aux);
            newBuf[index] = ESC;
            index++;
            newBuf[index] = STUFF(FLAG);
            index++;
        }

        else if(buf[i] == ESC){
            aux++;
            newBuf = realloc(newBuf,aux);
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
    return aux;
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
    unsigned char buf;
    int resR= 0, resW=0;
    flag = 0;
    counter = 0;
    unsigned char controlField;

    resW = writeInPort(fd,frame,frameSize);

    alarm(3); // Set alarm to be trigerred in 3s

    do {
        if (flag){
            alarm(3);
            flag = FALSE;
            resW = writeInPort(fd,frame,frameSize);
        }
        resR = read(fd,&buf,1);
        //printf("byte rec: %x\n",buf);
        if (!resR) continue;
        stateMachineSender(buf,&currentState,&controlField);
    } while (currentState != STOP && counter < numTries);
    alarm (0); /*write successfull*/

    int nextSeqNum = (seqNum) ? 0:1;

    if (currentState == STOP) {
        if (controlField == (REJ(seqNum))) { /* retransmissão */
            return sendFrame(frame,frameSize);
        }
        if (controlField == (RR(nextSeqNum))) {
            seqNum = nextSeqNum;
            return resW;
        }
    }

    return -1;

}
