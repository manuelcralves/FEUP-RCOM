
#include "../include/receiver.h"

unsigned char* destuffing(const unsigned char* buf, int *bufSize) {
    unsigned char* newBuf = malloc(sizeof(char)*255);  

    newBuf[0] = FLAG;

    int index = 1;

    for (size_t i = 1; i < (*bufSize)-1; i++){
        if (buf[i] == ESC){
            i++;

            if (buf[i] == STUFF(FLAG)){
                newBuf[index] = FLAG;
                index++;
            }

            else {
                newBuf[index] = ESC;
                index++;
            }
        }

        newBuf[index] = buf[i];
        index++;
    }

    newBuf[index] = FLAG;
    *bufSize = index+1;
    return newBuf;
}