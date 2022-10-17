#include "../include/sender.h"

int sendControl(int fd, unsigned char controlField, unsigned char response) {
  
  if(sendMessage(fd, A_SND, controlField) < 0) return -1; // sends sender adress 
  
  if(receiveMessage(fd, A_RCVR, response) < 0) return -1; // receives response
  
  return fd;
}

