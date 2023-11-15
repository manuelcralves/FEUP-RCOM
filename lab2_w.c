// Write to serial port in non-canonical mode
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


enum STATE {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK,STOP}


volatile int STOP = FALSE;

#define FLAG 0x7e
#define A1 0x03
#define A2 0x01
#define SETUP 0x03
#define UA 0x07
#define BCC1 A1^SETUP
#define BCC2 A2^UA

enum state {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP};
int flag=0, conta=1;

void atende()
{
	printf("alarme # %d\n", conta);
	flag=1;
	conta++;
}

int main(int argc, char** argv)
{
    int fd,c, resR, resW;
    char buf[255];
    struct termios oldtio,newtio;
    unsigned char SET[5];
    
    enum state current_state = START;
    
    
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

    newtio.c_cc[VTIME]    = 5;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 1 chars received */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");
    (void) signal(SIGALRM, atende);
	
	 
    //Read input from stdin and send to serial port
    SET[0]=FLAG;
    SET[1]=A1;
    SET[2]=SETUP;
    SET[3]=BCC1;
    SET[4]=FLAG;
    
    printf("Sent SET msg...\n");
    resW = write(fd,SET,5);
    alarm(3);                 // activa alarme de 3s
	
    //Receive msg from serial port
    printf("Waiting for UA msg...\n");
    while (current_state != STOP && conta<4) {       /* loop for input */
      resR = read(fd,buf, 1);   /* returns after 1 chars have been input */
      
      if(flag){
		  alarm(3);                 // activa alarme de 3s
		  flag=0;
		  printf("Sent SET msg...\n");
		  printf("Waiting for UA msg...\n");
		  resW = write(fd,SET,5);
	  }
	  
	  if (!resR) continue;
	  printf("CURRENT_STATE: %d | 0x%x\n", current_state, buf[0]);
	
      switch (current_state) {
        case START:
            if(buf[0] == FLAG) current_state = FLAG_RCV;
            break;
        case FLAG_RCV:
            if(buf[0] == A2) current_state = A_RCV;
            else if (buf[0] != FLAG) current_state = START;
            break;
        case A_RCV:
            if(buf[0] == FLAG) current_state = FLAG_RCV;
            else if(buf[0] == UA) current_state = C_RCV;
            else current_state = START;
            break;
        case C_RCV:
            if(buf[0] == FLAG) current_state = FLAG_RCV;
            else if(buf[0] == BCC2) current_state = BCC_OK;
            else current_state = START;
            break;
        case BCC_OK:
            if(buf[0] == FLAG) current_state = STOP;
            else current_state = START;
            break;
        case STOP:
			break;
      }
    }
    printf("Terminating...\n");
    
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}
    
