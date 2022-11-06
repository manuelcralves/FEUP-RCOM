
#include "../include/receiver.h"

int destuffing(const unsigned char* buf, int bufSize,unsigned char* packet) {
    packet[0] = FLAG;

    int index = 1;

    for (size_t i = 1; i < (bufSize)-1; i++){
        if (buf[i] == ESC){
            i++;

            if (buf[i] == (STUFF(FLAG))){
                packet[index] = FLAG;
                index++;
                continue;
            }
            packet[index] = ESC;
            index++;
            continue;
        }

        packet[index] = buf[i];
        index++;
    }

    packet[index++] = FLAG;
    return index;
}

void stateMachineReceiver(char byteReceived, enum state * currentState, int * prevWasFlag) {
  switch (*currentState) {
    case START: {
      if(byteReceived == FLAG) {
       *currentState = FLAG_RCV;
       *prevWasFlag = 1;
       }
      break;
    }
    case FLAG_RCV: {
      if (byteReceived == FLAG){
        if (*prevWasFlag == 0) {
            *currentState = STOP;
        }
        else {break;}
      } 
      else *prevWasFlag = 0;
      break;
    }
    default: 
      break;
  }
}

int sendControlFrame_r(int fd, unsigned char adressField, unsigned char controlField) {
  
  unsigned char message[5];

  message[0] = FLAG;
  message[1] = adressField;
  message[2] = controlField;
  message[3] = BCC(adressField, controlField);
  message[4] = FLAG;

  int res = write(fd,message,5);
  return res;
}

int receiveControlFrame_r(int fd, unsigned char adressField, unsigned char controlField) {
    enum state current_state = START;
    unsigned char buf;
    int res = 0;
    do {
        res = read (fd,&buf,1);
        if(res < 0) return -1;
        if (!res) continue;
        stateMachine(buf, &current_state, adressField, controlField);
    } while(current_state != STOP);

    return res;
}


int receiveDataFrame_r(int fd,unsigned char* frame) {
    enum state currentState = START;
    unsigned char buf;
    int res =0, currentIndex= 0;
    int prevWasFlag = 0;

    do {
        res = read(fd,&buf,1);
        if (res  < 0) {
            return -1;
        }

        if (!res) continue;

        stateMachineReceiver(buf,&currentState,&prevWasFlag);
        if (prevWasFlag && currentState == FLAG_RCV) currentIndex = 0;
        if (currentState != START) frame[currentIndex++] = buf;

    } while (currentState != STOP);

    return currentIndex;
}