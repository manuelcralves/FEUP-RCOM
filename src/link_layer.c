// Link layer protocol implementation

#include "../include/link_layer.h"
#include "sender.h"
#include "receiver.h"
#include "../include/utils.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

struct termios oldtio;
struct termios newtio;
int fd;
extern unsigned int seqNum ;
int numTries;
int timeout;
LinkLayerRole myRole;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{   
    
    fd = open(connectionParameters.serialPort,O_RDWR | O_NOCTTY);

    myRole = connectionParameters.role;
    timeout = connectionParameters.timeout;
    numTries = connectionParameters.nRetransmissions;

    (void) signal(SIGALRM, alarmHandler);

    if (fd < 0) {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

    //Save current port settings
    if (tcgetattr(fd,&oldtio) == -1) {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio,0,sizeof(newtio));

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 1;
    newtio.c_cc[VMIN] = 0;

    tcflush(fd,TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1){
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");


    if (myRole == LlTx){
        if (sendControlFrame_t(fd,A_SND,SET,A_RCVR,UAKN) < 0) return -1;
        return fd;
    }

    if (myRole== LlRx) {
        if (receiveControlFrame_r(fd,A_SND,SET)<0) return -1;
        if (sendControlFrame_r(fd,A_RCVR,UAKN) < 0) return -1;
        return fd;
    }

    return -1;
    
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{   
    
    int newBufSize = bufSize + HEADER_BYTES;
    unsigned char newBuf[newBufSize];
    printf("seqNum : %d\n",seqNum);
    newBuf[0] = FLAG;
    newBuf[1] = A_SND;
    newBuf[2] = SEQ_NUM(seqNum);
    newBuf[3] = BCC(A_SND,newBuf[2]);

    unsigned char bccData = 0x00;
    for (size_t i = 0; i < bufSize;i++){
        unsigned char dataByte = buf[i];
        printf("%x ",dataByte);
        newBuf[4+i] = dataByte;
        bccData = bccData ^ dataByte;
    }   

    newBuf[newBufSize-2] = bccData;
    newBuf[newBufSize-1] = FLAG;

    printf("\n-----\n");
    printf("antes de dar stuff tamanho : %d\n",newBufSize);

    unsigned char *stuffed = malloc(newBufSize);
    newBufSize = stuffing(newBuf,newBufSize,stuffed);

    printf("depois de dar stuff tamanho : %d\n",newBufSize);
    printf("-----\n");

    int res = sendDataFrame_t(stuffed,newBufSize);

    free(stuffed);
    return res;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    int res;
    unsigned char buffer[1000];

    res = receiveDataFrame_r(fd,buffer); /*receiveFrame puts the frame in the buffer var and returns its size*/
    
    if (res < 0) {
        return -1; 
    }
    printf("destuffing...\n");
    res = destuffing(buffer,res,packet);
    printf("o numero de seq é : %d\n",seqNum);
    for (int i = 0; i < res;i++) {printf("%x ",packet[i]);}
    printf("\n");
    unsigned char prevSequenceNumber = (seqNum) ? 0:1;

    /*Wrong header - ignore*/
    if (isHeaderWrong(packet,seqNum)) return 0;

    /*Duplicate frame - ignore*/
    if(isDuplicate(fd,packet,seqNum)) return 0;

    /*Wrong seq Num - ignore*/
    if (isSeqNumWrong(packet,seqNum)) return 0;

    /*Wrong BCC*/
    if (isDataBccWrong(fd,packet,res,seqNum)) return 0;
    
    seqNum = prevSequenceNumber;
    
    /*Correct frame*/
    printf("manda RR do prox: %d\n",seqNum);
    sendControlFrame_r(fd,A_RCVR,RR(seqNum));

    return res - HEADER_BYTES;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{   
    switch(myRole) {
        case LlTx:
            if (sendControlFrame_t(fd,A_SND,DISC,A_RCVR,DISC) < 0) return -1; // the sender send the DISC command
            if (sendControlFrame_r(fd,A_SND,UAKN) < 0) return -1; // the sender sends UA , its calling the 'receiver' function because this sender doesn't expect to receive a response from the receiver
            sleep(1);
            break;
        case LlRx:
            if (receiveControlFrame_r(fd,A_SND,DISC) < 0) return -1; // the receiver receives the DISC command
            if (sendControlFrame_r(fd,A_RCVR,DISC) < 0) return -1; // the receiver send the response to the sender

            if (receiveControlFrame_r(fd,A_SND,UAKN) < 0) return -1; // the receiver receives UA
            break;
    }
    
    if ( tcsetattr(fd, TCSANOW, &oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    //TODO 

    if (showStatistics) {
        printf("show statistics");
    }

    close(fd);

    return 1;
}
