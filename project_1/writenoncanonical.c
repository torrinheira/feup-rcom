/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include "states.h"

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS0"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define M_FLAG 0x7e
#define M_A_SND 0x01
#define M_C_SND 0x07
#define TRANSMITTER 0
#define RECEIVER 1
#define MAX_SIZE 255

volatile int STOP=FALSE;

void sendSet(int fd);
int readUa(int fd);
void failed(void);
int llopen(int fd,int device);
int llwrite(int fd, char * buffer, int length);

int expired;
int tries = 0;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int a[3]={1,2,3};
    int i, sum = 0, speed = 0;

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

    newtio.c_cc[VTIME]    = 2;   /* inter-character timer unused */      //iniciais eram 0\n1
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



  /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) pr�ximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
/* -------------------------------------------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/
/* -------------------------------------ESTABELECER LIGAÇÃO-----------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/




		if(llopen(fd,TRANSMITTER) < 0){
			perror("could not establish connection");
			exit(-1);
	}
	  //-------------------------------------



	  //-------------------------------------

	sleep(1);
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }
    close(fd);
    return 0;
}

//Funcoes Auxiliares

int llwrite(int fd, char * buffer, int length){
	
	bool not_done = true;
	int index = 0;
	while(not_done){
	
	unsigned char * message = createsInfoMessage();

	index = fillsData(message,buffer, length,index);

	/*
		
		
		CODIGO QUE ENVIA A MENSAGEM E ESPERA PELO ACK
		sera um WHILE



	*/


	if(index == (length -1))
		not_done = false;


	}
	
	
}


int llopen(int fd,int device){

	if(device == TRANSMITTER){
		
		int success = 0;

	

		while(tries < 3){
			alarm(3);
			(void) signal(SIGALRM, failed);	
			sendSet(fd);
			if(-1 == readUa(fd))
				continue;
			success = 1;
			break;
		}


		(void) signal(SIGALRM, SIG_IGN);

		printf("UA received. Connection estabilished \n\n");
	
		return fd;
		}
		
		else
			return -1;
}

unsigned char * createsInfoMessage(){

	static int alternate = 0;

	unsigned char* buffer[255];

	buffer[0] = M_FLAG;

	buffer[1] = 0x03;

	if(altenate == 0)
		buffer[2] = 0x00;
	else if(alternate == 1)
		buffer[2] = 0x40;

	buffer[3] = buffer[1]^buffer[2];


	if(alternate == 0)
		alternate = 1;
	else if(alternate == 1)
		alternate = 0;
	else return -1;

	return buffer;
	
}

//RETURNS INDEX OF DATA
int fillsData(unsigned char* message, char* buffer, int length, int index){
		

	int i;
	unsigned char bcc = buffer[index];

	for(i = 4; i <= 253 ; i++){
		message[i] = buffer[index];

		//para BCC2
		if(i > 4)
			bcc = bcc^message[i]; 

		index++;

	if(index == (length -1)){// -1?
		i++;		
		break;
		}		

	}
	message[i] = bcc;
	message[i+1] = M_FLAG;
	return index;
}


void sendSet(int fd){

    unsigned char message[5] = {0x7e, 0x03, 0x03, 0x03^0x03,0x7e};
    write(fd, message, 5);
}

int readUa(int fd){


	struct state_machine* st = create_state_machine();
	unsigned char buffer[8];
	bool flag = true;

	bool toRead = true;

	while(flag){



		if(toRead){
			if(0 == (read(fd, buffer, 1)))
				return -1;
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
				else if(*buffer == M_A_SND)
					setCurrentEvent(st, A);
				else setCurrentEvent(st, OTHER);

			break;

			case A_RCV:
				if(*buffer == M_FLAG)
					setCurrentEvent(st, FLAG);
				else if(*buffer == M_C_SND)
					setCurrentEvent(st, C);
				else setCurrentEvent(st, OTHER);

			break;

			case C_RCV:
				if(*buffer == M_FLAG)
					setCurrentEvent(st, FLAG);
				else if(*buffer == (0x01^0x07))
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
				return 1;
			break;
		}

	}	

}

void failed(){
	tries++;
	if(tries == 3)
		exit(-1);
	expired = 1;
	(void) signal(SIGALRM, SIG_IGN);
	printf("Time expired!\n");
}

