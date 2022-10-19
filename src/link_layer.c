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

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    int fd = open (connectionParameters.serialPort,O_RDWR | O_NOCTTY);
    role = connectionParameters.role;


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
    // TODO
    
    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
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
