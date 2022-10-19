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
int fd;
LinkLayerRole role;
extern unsigned int seqNum;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    int fd = open (connectionParameters.serialPort,O_RDWR | O_NOCTTY);
    role = connectionParameters.role;

    (void) signal(SIGALRM, alarmHandler);

    if (fd < 0) {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

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
    newtio.c_cc[VTIME] = 5;
    newtio.c_cc[VMIN] = 0;

    tcflush(fd,TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1){
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    if (tcsetattr(fd,TCSANOW,&oldtio) == -1){
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);

    return 1;
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

    if ((res = receiveFrame(buffer)) < 0) return -1; /*receiveFrame puts the frame in the buffer var and returns its size*/

    unsigned char* destuffed;
    destuffed = destuffing(buffer,&res); 

    if (isHeaderWrong(destuffed)) return 0;

    if(isDuplicate(destuffed)) return 0;

    if (isSeqNumWrong(destuffed)) return 0;

    if (isDataBccWrong(destuffed,res)) return 0;

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
    switch(role) {
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
