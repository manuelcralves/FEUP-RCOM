#include "../include/utils.h"

extern unsigned int flag;
extern int numTries;
extern int counter;
extern int timeout;

void stateMachine(char byteReceived, enum state * currentState, unsigned char addressField, unsigned char controlField) {
  switch (*currentState) {
    case START:
      if(byteReceived == FLAG) *currentState = FLAG_RCV;
      break;
    case FLAG_RCV:
      if(byteReceived == addressField) *currentState = A_RCV;
      else if (byteReceived != FLAG) *currentState = START;
      break;
    case A_RCV:
      if(byteReceived == FLAG) *currentState = FLAG_RCV;
      else if(byteReceived == controlField) *currentState = C_RCV;
      else *currentState = START;
      break;
    case C_RCV:
      if(byteReceived == FLAG) *currentState = FLAG_RCV;
      else if(byteReceived == (BCC(addressField, controlField))) *currentState = BCC_OK;
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

int sendInfoFrame_r(int fd, unsigned char adressField, unsigned char controlField) {
  
  unsigned char message[5];

  message[0] = FLAG;
  message[1] = adressField;
  message[2] = controlField;
  message[3] = BCC(adressField, controlField);
  message[4] = FLAG;
  
  int res = write(fd,message,5);
  printf("res:%d\n",res);
  return res;
}

int sendFrame1(int fd, unsigned char adressField,unsigned char controlField) {
    enum state currentState = START;
    unsigned char buf;
    int resR= 0, resW=0;
    flag = 0;
    counter = 0;
    unsigned char frame[5];
    frame[0] = FLAG;
    frame[1] = adressField;
    frame[2] = controlField;
    frame[3] = BCC(adressField, controlField);
    frame[4] = FLAG;

    resW = writeInPort(fd,frame,5);
    
    alarm(3); // Set alarm to be trigerred in 3s

    do {
        if (flag){
            alarm(3);
            flag = FALSE;
            resW = writeInPort(fd,frame,5);
        }
        resR = read(fd,&buf,1);
        printf("byte rec: %x\n",buf);
        printf("resR : %d\n",resR);
        printf("counter : %d\n",counter);
        printf("state : %s\n",getState(currentState));
        printf("-----\n");
        if (!resR) continue;
        stateMachine(buf,&currentState,0x01,0x07);
    } while (currentState != STOP && counter < numTries);
    alarm (0); /*write successfull*/

    return resW;
    /*
    int nextSeqNum = (seqNum) ? 0:1;

    if (currentState == STOP) {
        if (controlField == (REJ(seqNum))) { 
            return sendFrame(frame,frameSize);
        }
        if (controlField == (RR(nextSeqNum))) {
            seqNum = nextSeqNum;
            return resW;
        }
    }*/
    printf("fehcou\n");

    return -1;

}

int receiveInfoFrame(int fd, unsigned char adressField, unsigned char controlField) {
    enum state current_state = START;
    unsigned char buf;
    int res = 0;
    do {
        res = read (fd,&buf,1);
        printf("buf: %x\n",buf);
        if(res < 0) return -1;
        if (!res) continue;
        stateMachine(buf, &current_state, adressField, controlField);
    } while(current_state != STOP && counter < numTries);

    return res;
}

int isHeaderWrong(unsigned char *buf, int seqNum) {
  if (buf[0] != FLAG || buf[1] != A_SND || buf[3] != (BCC(A_SND,SEQ_NUM(seqNum)))){
    return 1;
  }
  return 0;
}

int isDuplicate(int fd,unsigned char *buf,int seqNum) {
  int prevSeqNum = 0;
  if (seqNum == 0) prevSeqNum = 1;

  if (buf[2] == SEQ_NUM(prevSeqNum)) {
    sendInfoFrame_r(fd,A_RCVR,RR(seqNum));
    return 1;
  }
  return 0;
}

int isSeqNumWrong(unsigned char *buf,int seqNum) {
  if (buf[2] != SEQ_NUM(seqNum)) return 1;
  return 0;
}

int isDataBccWrong(int fd,unsigned char *buf, int bufSize,int seqNum) {
  unsigned char dataBcc = 0x00;

  for (size_t i = 4; i < bufSize-2;i++){
    dataBcc ^= buf[i];
  }

  if (buf[bufSize-2] != dataBcc){
    sendInfoFrame_r(fd,A_RCVR,REJ(seqNum));
    return 1;
  }

  return 0;
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


const char* getState (enum state s){
  switch(s)
  {
    case START : return "0";
    case FLAG_RCV : return "1";
    case A_RCV : return "2";
    case C_RCV : return "3";
    case BCC_OK : return "4";
    case STOP : return "5";

  }
}