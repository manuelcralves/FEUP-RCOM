// Link layer protocol implementation

#include "link_layer.h"
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
    newtio.c_cflag = 0;
    newtio.c_cc[VTIME] = 1;
    newtio.c_cc[VMIN] = 0;

    tcflush(fd,TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1){
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");


    if (myRole == LlTx){
        if (sendInfoFrame(fd,A_SND,SET) < 0) return -1;
        if (receiveInfoFrame(fd,A_RCVR,UAKN) < 0) return -1;
        return fd;
    }

    if (myRole== LlRx) {
        if (receiveInfoFrame(fd,A_SND,SET)<0) return -1;
        if (sendInfoFrame(fd,A_RCVR,UAKN) < 0) return -1;
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

    newBuf[0] = FLAG;
    newBuf[1] = A_SND;
    newBuf[2] = SEQ_NUM(seqNum);
    newBuf[3] = BCC(A_SND,newBuf[2]);

    unsigned int bccData = 0x0;
    for (size_t i = 0; i < bufSize;i++){
        unsigned char dataByte = buf[i];
        newBuf[4+i] = dataByte;
        bccData = bccData ^ dataByte;
    }   

    newBuf[newBufSize-2] = bccData;
    newBuf[newBufSize-1] = FLAG;

    unsigned char *stuffed = malloc(sizeof(char)*(newBufSize*3));
    stuffed = stuffing(newBuf,&newBufSize);

    int res = sendFrame(stuffed,newBufSize);

    free(stuffed);
    return res;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    int res;
    unsigned char buffer[255];
    printf("llread\n");
    printf("fd: %d\n\n",fd);
    res = receiveFrame(fd,buffer);
    if (res < 0) return -1; /*receiveFrame puts the frame in the buffer var and returns its size*/
    printf("destuffing...\n");
    res = destuffing(buffer,res,packet); 

    if (isHeaderWrong(packet)) return 0;

    if(isDuplicate(fd,packet)) return 0;

    if (isSeqNumWrong(packet)) return 0;

    if (isDataBccWrong(fd,packet,res)) return 0;

    if (seqNum == 0) seqNum = 1;
    if (seqNum == 1) seqNum = 0;


    sendInfoFrame(fd,A_RCVR,RR(seqNum));

    return res - HEADER_BYTES;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    switch(myRole) {
        case LlTx:
            if (sendInfoFrame(fd,A_SND,DISC) < 0) return -1; // the sender send the DISC command
            if (receiveInfoFrame(fd,A_RCVR,DISC) < 0) return -1; // the sender receives the DISC command sent by the receiver

            if (sendInfoFrame(fd,A_SND,UAKN) < 0) return -1; // the sender sends UA 
            sleep(1);
            break;
        case LlRx:
            if (receiveInfoFrame(fd,A_SND,DISC) < 0) return -1; // the receiver receives the DISC command
            if (sendInfoFrame(fd,A_RCVR,DISC) < 0) return -1; // the receiver send the response to the sender

            if (receiveInfoFrame(fd,A_SND,UAKN) < 0) return -1; // the receiver receives UA
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
