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
#define M_A_REC 0x03
#define M_C_SND 0x07
#define DISC 0x0B
#define TRANSMITTER 0
#define RECEIVER 1
#define MAX_SIZE 255
#define ESCAPE 0x7d
#define ESCAPE_FLAG 0x5d
#define FLAG_ESC 0x5e
#define MAX_TRIES 3
#define ACCEPTED 1		//ACK sent correctly
#define REJECTED -1		//ACK wrong

#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81

volatile int STOP=FALSE;

void sendSet(int fd);
int readUa(int fd);
void failed(void);
int llopen(int fd,int device);
int llwrite(int fd, char * buffer, int length);
void message_stuffing(char* buffer);
void destuffing(char* buffer);
unsigned char readControlMessageC(int fd);

int createsInfoMessage(unsigned char * buffer);

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
	

	// char * buffer[9]= {0x7e,0x7e,0x06,0x07,0x7d,0x00,0x12,0x24,0xe1};
	char * buffer = "hey, hey, tres a cinco do sete de 900";

	int size = sizeof(char) * strlen(buffer);

	message_stuffing(buffer);

	llwrite(fd, buffer, size); // o buffer vai ter o ficheiro a mandar completo

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    close(fd);
    return 0;
}

/* -------------------------------------------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/
/* -------------------------------------FUNÇÕES AUXILIARES------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/
/* -------------------------------------------------------------------------------------------------------------*/

//llwrite é para cada trama, não 1 para todos
//BUFFER JA ESTA STUFFED
int llwrite(int fd, char * buffer, int length){
	
	printf("%s\n", buffer);

	int bytes_written = 0;
	int index = 0;
	int answer = REJECTED;
	unsigned char ack;
	unsigned char* message = malloc(255);


		int alternate = createsInfoMessage(message);

		index = fillsData(message,buffer,length,index);

		write(fd,message,43);

		//TODO: calcular direito o tamanho da mensagem e substituir pelo 44 (que está certo)
		
		
		//agora é preciso enviar a informação para o reader e tratar da questão do timeout 
		
	
		ack = readControlMessageC(fd);
		
		printf("%c\n",ack);
	
		if(ack == RR0 && alternate == 0)
			answer = ACCEPTED;
		else if(ack == RR1 && alternate == 1)
			answer = ACCEPTED;

		if(answer == ACCEPTED){
			printf("Acknowledge!\n");
		}else printf("nao recebeu ack\n");
		
	
	

	
	bytes_written = index;
	return bytes_written;
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

int createsInfoMessage(unsigned char* buffer){
	static int alternate = 0;


	buffer[0] = M_FLAG;

	buffer[1] = 0x03;


	if(alternate == 0){
		buffer[2] = 0x00;
		buffer[3] = 0x03^0x00;
	}
	else if(alternate == 1){
		buffer[2] = 0x40;
		buffer[3] = 0x03^0x40;
	}
	if(alternate == 0)
		alternate = 1;
	else if(alternate == 1)
		alternate = 0;
	else return -1;
	return alternate;
	
}

void message_stuffing(char* buffer){
	
	unsigned char tempo[255];
	unsigned char * temp_buff = tempo;
	int j = 0;
	

	for(size_t i = 0; i < strlen(buffer) ;i++ ){

		if(buffer[i] == M_FLAG){
			temp_buff[j] = ESCAPE;
			temp_buff[j+1] = FLAG_ESC;	
			j = j + 2;
		}
		else if(buffer[i] == ESCAPE){
			temp_buff[j] = ESCAPE;
			temp_buff[j+1] = ESCAPE_FLAG;
			j = j + 2;
		}
		else{
			temp_buff[j] = buffer[i];
			j++;
		}
	}

	buffer = tempo;
}

//RETURNS INDEX OF DATA
int fillsData(unsigned char* message, char* buffer, int length, int index){
		
	//message=malloc(255);
	int i;
	char tempo[255];
	int j = 0;
	char * temp = tempo;

	
	for(i = 4; i <= 253 ; i++){

		message[i] = buffer[index];
		temp[j] = buffer[index];

		if(index == (length ))	
			break;
				
		index++;
		j++;
	}

	printf("%s\n", temp);
	destuffing(temp);
	

	char bcc2 = temp[0];

	for (int k = 1; k < strlen(temp); k++){
		bcc2 = bcc2 ^ temp[k];
	}

	
	message[i] = bcc2; 
	message[i+1] = M_FLAG;

	return index;
}

void destuffing(char* temp){

	char* buf[255];
	int j = 0;

	int size = sizeof(char) * strlen(temp);


	for(int i= 0; i < size;){

		if(temp[i] == ESCAPE && temp[i + 1] == FLAG_ESC){
			buf[j] = M_FLAG;
			i = i + 2;
		}
		else if(temp[i] == ESCAPE && temp[i + 1] == ESCAPE_FLAG){
			buf[j] = ESCAPE;
			i = i + 2;
		}
		else{
			buf[j] = temp[i];
			i++;
		}

	j++;
	}

	temp = buf;
}

void sendSet(int fd){

    unsigned char message[5] = {0x7e, 0x03, 0x03, 0x03^0x03,0x7e};
    write(fd, message, 5);
}

int readUa(int fd){


	struct state_machine* st = create_state_machine("set"); //missing state machine type
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

unsigned char readControlMessageC(int fd)
{
  int state = 0;
  unsigned char c;
  unsigned char C;

  while (state != 5)
  {
    read(fd, &c, 1);
    switch (state)
    {
    //recebe FLAG
    case 0:
      if (c == M_FLAG)
        state = 1;
      break;
    //recebe A
    case 1:
      if (c == M_A_REC)
        state = 2;
      else
      {
        if (c == M_FLAG)
          state = 1;
        else
          state = 0;
      }
      break;
    //recebe c
    case 2:
      if (c == RR0 || c == RR1 || c == REJ0 || c == REJ1 || c == DISC)
      {
        C = c;
        state = 3;
      }
      else
      {
        if (c == M_FLAG)
          state = 1;
        else
          state = 0;
      }
      break;
    //recebe BCC
    case 3:
      if (c == (M_A_REC ^ C))
        state = 4;
      else
        state = 0;
      break;
    //recebe FLAG final
    case 4:
      if (c == M_FLAG)
      {
        alarm(0);
        state = 5;
        return C;
      }
      else
        state = 0;
      break;
    }
  }
  return 0xFF;
}

