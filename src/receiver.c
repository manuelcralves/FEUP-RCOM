
#include "../include/receiver.h"

int receiveControl(int fd, unsigned char controlField, unsigned char response) {

  if(receiveMessage(fd, A_SND, controlField) < 0) return -1;

  if(sendMessage(fd, A_RCVR, response) < 0) return -1;
  
  return fd;
}