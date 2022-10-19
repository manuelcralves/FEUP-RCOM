#include "../include/sender.h"

unsigned char* stuffing(const unsigned char *buf, int *bufSize) {
    unsigned char *newBuf = malloc(sizeof(char)*255);

    newBuf[0] = FLAG;
    
    int index = 1;

    for (size_t i = 1; i < bufSize -1; i++){
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
    bufSize = index+1;
    return newBuf;
}
