#include "../include/utils.h"

extern unsigned int seqNum;
extern int fd;

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

int sendInfoFrame(int fd, unsigned char adressField, unsigned char controlField) {
  unsigned char message[5];
  
  message[0] = FLAG;
  message[1] = adressField;
  message[2] = controlField;
  message[3] = BCC(adressField, controlField);
  message[4] = FLAG;
  
  return write(fd, message, 5);
}

int receiveInfoFrame(int fd, unsigned char adressField, unsigned char controlField) {
    enum state current_state = START;
    char buf[255];
    int res = 0;
    do {
        if((res = read(fd, buf, 1)) < 0) return -1;
        if (!res) continue;
        stateMachine(buf[0], &current_state, adressField, controlField);
    } while(current_state != STOP);

    return res;
}

int isHeaderWrong(unsigned char *buf) {
  if (buf[0] != FLAG || buf[1] != A_SND || buf[3] != (BCC(A_SND,SEQ_NUM(seqNum)))){
    return 1;
  }
  return 0;
}

int isDuplicate(unsigned char *buf) {
  int prevSeqNum = 0;
  if (seqNum == 0) prevSeqNum = 1;

  if (buf[2] == SEQ_NUM(prevSeqNum)) {
    sendInfoFrame(fd,A_RCVR,RR(seqNum));
    return 1;
  }
  return 0;
}

int isSeqNumWrong(unsigned char *buf) {
  if (buf[2] != SEQ_NUM(seqNum)) return 1;
  return 0;
}

int isDataBccWrong(unsigned char *buf, int bufSize) {
  unsigned char dataBcc = 0x00;
  for (size_t i = 4; i < bufSize-2;i++){
    dataBcc ^= buf[i];
  }

  if (buf[bufSize-2] != dataBcc){
    sendInfoFrame(fd,A_RCVR,REJ(seqNum));
    return 1;
  }

  return 0;
}
