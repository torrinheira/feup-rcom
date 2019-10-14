/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "states.h"

#define M_FLAG 0x7e
#define M_A_REC 0x03
#define M_C_REC 0x03
#define BAUDRATE B9600
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define TRANSMITTER 0
#define RECEIVER 1
#define ESCAPE 0x7d
#define ESCAPE_FLAG 0x5d
#define FLAG_ESC 0x5e

volatile int STOP=FALSE;

void readSET(int fd);
void sendUA(int fd);
void readInfoZero(int fd);
int llopen(int porta, int device);
int llread(int fd, char * buffer);

int main(int argc, char** argv)
{

    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
	char buf_m[255];
	
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


  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) pr�ximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);
    

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

	// ----------------- READS SET AND SENDS UA ------------------

	if(llopen(fd, RECEIVER) < 0){
		perror("could not establish connection");
		exit(-2);
	}
	
	sleep(1);

	printf("%d", 0);
	//chamar llread num ciclo 
	char * buffer;
	llread(fd, buffer); 

	printf("%s\n", buffer);

 	tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}

int llopen(int fd, int device){
	if(device == RECEIVER){
		
		readSET(fd);

		printf("SET received, everything is OK\n");
		printf("Sending UA ...\n");

		sendUA(fd);
		return fd;
	} else return -1;
}

// Reads SET from writenoncanonical.c
void readSET(int fd){

	struct state_machine* st = create_state_machine("set");

	unsigned char buffer[8];
	bool flag = true;

	bool toRead = true;

	while(flag){
	
		// checks if it should keep on reading
		if(toRead){
			read(fd, buffer, 1);
		}

		message_handler(st);

		switch(getCurrentState(st)){

			case START:
				if(*buffer == M_FLAG)
					setCurrentEvent(st, FLAG);
			break;

			case FLAG_RCV:
				if(*buffer == M_FLAG)
					setCurrentEvent(st, FLAG);
				else if(*buffer == M_A_REC)
					setCurrentEvent(st, A);
				else setCurrentEvent(st, OTHER);

			break;

			case A_RCV:
				if(*buffer == M_FLAG)
					setCurrentEvent(st, FLAG);
				else if(*buffer == M_C_REC)
					setCurrentEvent(st, C);
				else setCurrentEvent(st, OTHER);

			break;

			case C_RCV:
				if(*buffer == M_FLAG)
					setCurrentEvent(st, FLAG);
				else if(*buffer == 0x00)
					setCurrentEvent(st, BCC_OK);
				else setCurrentEvent(st, OTHER);

			break;

			case BCC_OK:
				if(*buffer == M_FLAG){
					setCurrentEvent(st, FLAG);
					toRead = false;
				}

				else setCurrentEvent(st, OTHER);

			break;

			case STOP_RCV:
				flag = false;

			break;
		}

	}

}

// sends UA
void sendUA(int fd){

	unsigned char message[5] = {0x7e, 0x01, 0x07, 0x01 ^ 0x07, 0x7e};
	write(fd, message, 5);
	printf("UA sent\n");
}


void readInfoZero(int fd){

	struct state_machine* st = create_state_machine("info");

	unsigned char buffer[8];
	bool flag = true;

	bool toRead = true;

	while(flag){
	
		// checks if it should keep on reading
		if(toRead){
			read(fd, buffer, 1);
		}

		message_handler(st);

		switch(getCurrentState(st)){

			case START:
				if(*buffer == M_FLAG)
					setCurrentEvent(st, FLAG);
			break;

			case FLAG_RCV:
				if(*buffer == M_FLAG)
					setCurrentEvent(st, FLAG);
				else if(*buffer == M_A_REC)
					setCurrentEvent(st, A);
				else setCurrentEvent(st, OTHER);

			break;

			case A_RCV:
				if(*buffer == M_FLAG)
					setCurrentEvent(st, FLAG);
				else if(*buffer == M_C_REC)
					setCurrentEvent(st, C);
				else setCurrentEvent(st, OTHER);

			break;

			case C_RCV:
				if(*buffer == M_FLAG)
					setCurrentEvent(st, FLAG);
				else if(*buffer == 0x00)
					setCurrentEvent(st, BCC_OK);
				else setCurrentEvent(st, OTHER);

			break;

			case BCC_OK:
				if(*buffer == M_FLAG){
					setCurrentEvent(st, FLAG);
					toRead = false;
				}

				else setCurrentEvent(st, OTHER);
			
			case DATA_REC:
				if(*buffer == M_FLAG){
					setCurrentEvent(st, FLAG);
					toRead = false;
				}


			break;

			case STOP_RCV:
				flag = false;

			break;
		}

	}

}


int llread(int fd, char * buffer){

	// se passar esta função a mensagem está certa
	printf("1");
	readInfoZero(fd);
	printf("2");
	char * msg;
	char * msg_i;
	bool flag = true;
	int counter = 0;

	int i = 0;
	
	while(flag){
		
		read(fd, msg, 1);
		if(counter != 0 && counter != 1 && counter != 2 && counter != 3){
			
			if(msg[counter] == M_FLAG){
				flag = false;
			}
			else{
				msg_i[i] = msg[counter];
				i++;
			}
			
		}
				
		counter++;
	}
	
	int j = 0;
	for(unsigned int i = 0; i < strlen(msg_i); ){

		if(msg_i[i] == ESCAPE && msg_i[i + 1] == FLAG_ESC){
			buffer[j] = M_FLAG;
			i = i + 2;
		}
		else if(msg_i[i] == ESCAPE && msg_i[i + 1] == ESCAPE_FLAG){
			buffer[j] = ESCAPE;
			i = i + 2;
		}
		else{
			buffer[j] = msg_i[i];
			i++;
		}

		j++;
	}

}

