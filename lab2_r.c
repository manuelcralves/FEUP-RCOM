// Read from serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h> 

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256


#define FALSE 0
#define TRUE 1
#define FLAG    0x7E
#define SETUP   0x03
#define DISC    0x0B
#define UAKN    0x07
#define A1      0x03
#define BCC1    A1 ^ SETUP
#define A2      0x01
#define BCC2    A2 ^ UAKN

volatile int STOP = FALSE;


enum state{START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP};

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    
    enum state current_state = START; 
      
    
    unsigned char UA[5];
    UA[0] = FLAG;
    UA[1] = A2;
    UA[2] = UAKN;
    UA[3] = BCC2;
    UA[4] = FLAG;
    
    
    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */


    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    printf("RECEIVED:\n");
    while (current_state != STOP) {       /* loop for input */
      res = read(fd,buf, 1);   /* returns after 1 chars have been input */
      
      switch (current_state) {
        case START:
            if(buf[0] == FLAG) current_state = FLAG_RCV;
            break;
        case FLAG_RCV:
            if(buf[0] == A1) current_state = A_RCV;
            else if (buf[0] != FLAG) current_state = START;
            break;
        case A_RCV:
            if(buf[0] == FLAG) current_state = FLAG_RCV;
            else if(buf[0] == SETUP) current_state = C_RCV;
            else current_state = START;
            break;
        case C_RCV:
            if(buf[0] == FLAG) current_state = FLAG_RCV;
            else if(buf[0] == BCC1) current_state = BCC_OK;
            else current_state = START;
            break;
        case BCC_OK:
            if(buf[0] == FLAG) current_state = STOP;
            else current_state = START;
            break;
      }
      
      printf("CURRENT_STATE: %d | 0x%x\n", current_state, buf[0]);

    }
    
    printf("SENT:\n");
    res = write(fd, UA, 5);
    for(int i = 0; i < 5; i++) {
        printf("0x%x\n", UA[i]);
    }
    
    sleep(1);

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
