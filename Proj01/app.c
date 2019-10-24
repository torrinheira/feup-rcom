#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "flags.h"
#include "protocolo.h"
#include "auxiliar.h"

//a função main será exclusiva e estará presente neste mesmo ficheiro
int main(int argc, char** argv){


    struct termios oldtio,newtio; 
    int fd = open(argv[1], O_RDWR | O_NOCTTY ); //abertura de um file descriptor associado a um ficheiro passado como argumento na consola

    if (fd <0) {
        printf("Error opening FILE DESCRIPTOR\n");
        exit(-1);
    }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }


    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }


/*
*
*  ESTABELICIMENTO DA COMUNICAÇÃO 
*   
*   declarar porta de série a usar, 0,1 ou 2
*   Pode ser do tipo WRITER or READER
*   Se for writer tem de obrigatoriamente ter um diretório do ficheiro se for reader nao
*
*/

FILE *file; //associado ao ficheiro que vai ser aberto para transmitir de um computador para outro
int comunication_type;

//processar argumentos
    if (((strcmp("/dev/ttyS0", argv[1])!=0) && (strcmp("/dev/ttyS1", argv[1])!=0) && (strcmp("/dev/ttyS2", argv[1])!=0))){
	    
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
	    exit(1);
	}


    if(strcmp(argv[2], "sender") == 0){
        comunication_type = TRANSMITTER;
        if(argc != 4){
            printf("Must be specified a file path to transmit\n");
            exit(1);
        }
    
    }
    else if(strcmp(argv[2], "receiver") == 0){
        comunication_type = RECEIVER;
    }
    else{
        printf("argv[2] must be sender or receiver");
        exit(1);
    }

/* llopen para estabelecer ligaçoes */
/* depende do tipo, transmitter or receiver*/

    if (comunication_type == TRANSMITTER){

        if ((file = fopen(argv[3], "rb")) == NULL)
            printf("failed to open file\n");
        if (llopen(fd, comunication_type) == -1)
            printf("failed to open connect with receiver\n");

        fflush(NULL);

        char buffer[MAX_SIZE];
        char *payload;
        char *stuffed;
        int size;
        short sequence_number = 0;
        char *control = control_frame(argv[3], file, 1, &size); //START frame
        stuffed = stuffing(control, &size);
        
        if (llwrite(fd, stuffed, size) == -1) //Send file info
            exit(1);

        printf("A enviar...\n");

        int enviado = 0;
        int file_size = getFileSize(file);

        while ((size = fread(buffer, sizeof(char), MAX_SIZE, file)) > 0){ //Lê fragmento do ficheiro


            payload = data_packet(sequence_number++, &size, buffer); //Adiciona campo de controlo, número de sequência, tamanho da payload
            stuffed = stuffing(payload, &size);                 //Transparência e adiciona BCC2

            if (llwrite(fd, stuffed, size) == -1) 
                exit(1);

            printf("um ciclo feito\n");
        }
        
        control = control_frame(argv[3], file, 0, &size); //END frame
        stuffed = stuffing(control, &size);

        if (llwrite(fd, stuffed, size) == -1) //Send END frame
            printf("failed to send END frame\n");

    }
    else if(comunication_type == RECEIVER){
        
        // establishes connection
        if(llopen(fd, RECEIVER) < 0){
		    perror("could not establish connection\n");
		    exit(-2);
	    }


        // receives the control package and saves the file name and its size
        int control_flag = 1;
        int length;
        char control_p[500];
        char* file_name;
        int file_size = 0;

        char stuffed[500];
        char* buffer;
        char* destuffed;

        while(control_flag){

            length = llread(fd, control_p);
            
            file_name = get_info(control_p, &file_size);
            if(file_name != NULL){
					printf("File Name: %s\nFile Size: %d\n", file_name, file_size);
					send_RR(fd);
					file=fopen(file_name, "wb");	
					control_flag = 0;
				}
				else{
                    printf("file name null\n");
					send_REJ(fd);
                }

        }

        
        //parser do pacote de dados
        printf("receiving ...\n");
        while( (length = llread(fd, stuffed) )> 0){
			
			if(stuffed[0]==END){
				
				buffer = verify_bcc2(stuffed, &length);	
				if(buffer == NULL)
					send_REJ(fd);
				else{
					send_RR(fd);
					break;
				}
			}	
			else{
				destuffed = verify_bcc2(stuffed, &length);								//Faz destuff e verifica o BCC2
				
				if(destuffed == NULL)
						send_REJ(fd);
				else{
					buffer = rem_data_packet(destuffed, &length);							//Remove Header 
					send_RR(fd);
					fwrite(buffer,1,length,file);
				}
				
			}
			
		}	
        

        
    }
    else{
        printf("unrecognized type of communication\n");
        exit(1);
    }






/*------------------------------------------------------------ TERMINAÇÃO DA APLICAÇÃO ----------------------------------------------------------------------------------*/


    llclose(fd, comunication_type);

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    close(fd);
    return 0;
}
