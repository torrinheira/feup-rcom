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
#include "flags.h"


volatile int STOP= false;

void readSET(int fd);
void sendUA(int fd);
void readInfoZero(int fd);
int llopen(int porta, int device);
int llread(int fd, char * buffer);
void destuffing(unsigned char * msg, unsigned char * final);
void separate(int counter, unsigned char * msg, unsigned char * buffer);

int m_bit;

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


	sleep(2);

	//chamar llread num ciclo
	char * buffer;
	llread(fd, buffer);

	//printf("%s\n", buffer);

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
		printf("UA sent\n");


		return fd;
	}
	else
		return -1;
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
				else if(/**buffer == M_C_REC*/*buffer == 0x00 || *buffer == 0x40){
          // here we take the desired bit from C flag to be then used in the R acknowledge
          if(*buffer == 0x00){
            m_bit = 0;
          }
          else if(*buffer == 0x40){
            m_bit = 1;
          }

					setCurrentEvent(st, C);
        }
				else setCurrentEvent(st, OTHER);

			break;

			case C_RCV:
				if(*buffer == M_FLAG)
					setCurrentEvent(st, FLAG);
				else if(*buffer == 0x03 || *buffer == 0x43){
					toRead=false;
					setCurrentEvent(st, BCC_OK);
				}
				else setCurrentEvent(st, OTHER);

			break;

			case BCC_OK:
				if(*buffer == M_FLAG){
					setCurrentEvent(st, FLAG);
					toRead = false;
				}

				else
					setCurrentEvent(st, OTHER);

			case DATA_REC:
				/*
				if(*buffer == M_FLAG){
					setCurrentEvent(st, FLAG);
					toRead = false;
				}
				*/

				//setCurrentEvent(st, OTHER);
				flag = false;
				toRead= false;


			break;

			case STOP_RCV:
				printf("STOP_RCV\n");
				flag = false;

			break;
		}

	}

}


int llread(int fd, char * buffer){

	// se passar esta função a trama inicial está certa
	unsigned char msg[255];
	readInfoZero(fd);
	unsigned char *msg_pointer = msg;

	// supostamente no fd agora tem apenas o chunk de dados e as flags de terminação


	bool flag = true;
	int counter = 0;
	while(flag){

		read(fd, msg_pointer, 1);

		if(msg[counter] != 0x7e){
			counter++;
		}
		else{
			flag = false;
		}
		msg_pointer++;

	}


	char bcc2 = msg[counter - 1];
  printf("%c\n", bcc2);


	// separar a mensagem da trama final, o buf fica com a informacao isolada
	unsigned char buf[255];
	unsigned char *buf_pointer = buf;
	separate(counter, msg, buf_pointer);



	// dar destuffing ao msg e verificar se o BCC2 e a FLAG estao corretas
	unsigned char final_msg[255];
  unsigned char *final = final_msg;
	destuffing(buf, final);
	printf("%s\n", final);


  // calcular bcc2 da mensagem obtida para mais tarde comparar com o bcc2 recebido
  char bcc2_comp = final[0];
  for(int k = 1; k < strlen(final); k++){
    bcc2_comp = bcc2_comp ^ final[k];
  }
  printf("%c\n", bcc2_comp);

	// enviar acknowledge
  tcflush(fd, TCIOFLUSH);

  /*
  unsigned char ack[5] = {M_FLAG, M_A_REC, RR1, M_A_REC ^ RR1, M_FLAG };
  unsigned char *ack_pointer = ack;
  */

  if(bcc2 == bcc2_comp){

    printf("acknowledged\n");
    printf("%d\n", m_bit);
    if(m_bit == 0){
      unsigned char ack[5] = {M_FLAG, M_A_REC, RR1, M_A_REC ^ RR1, M_FLAG };
      printf("%s\n", ack);
      write(fd, ack, 5);
    }
    else if(m_bit == 1){
      unsigned char ack[5] = {M_FLAG, M_A_REC, RR0, M_A_REC ^ RR0, M_FLAG };
      unsigned char *ack_pointer = ack;
      printf("%s\n", ack);
      write(fd, ack, 5);
    }

  }
  else{
    printf("%d\n", m_bit);
    if(m_bit == 0){
      unsigned char ack[5] = {M_FLAG, M_A_REC, REJ1, M_A_REC ^ REJ1, M_FLAG };
      write(fd, ack, 5);
    }
    else if(m_bit == 1){
      unsigned char ack[5] = {M_FLAG, M_A_REC, REJ0, M_A_REC ^ REJ0, M_FLAG };
      write(fd, ack, 5);
    }
  }



}

void destuffing(unsigned char * msg, unsigned char * final){

	int j = 0;
	for(unsigned int i = 0; i < strlen(msg); ){

		if(msg[i] == ESCAPE && msg[i + 1] == FLAG_ESC){
			final[j] = M_FLAG;
			i = i + 2;
		}
		else if(msg[i] == ESCAPE && msg[i + 1] == ESCAPE_FLAG){
			final[j] = ESCAPE;
			i = i + 2;
		}
		else{
			final[j] = msg[i];
			i++;
		}

		j++;
	}

}

void separate(int counter, unsigned char * msg, unsigned char * buffer){


	for(unsigned int i = 1; i < counter; i++){

		if(msg[i] != M_FLAG){
			buffer[i - 1] = msg[i - 1];
		}
		else{
			break;
		}

	}

}
