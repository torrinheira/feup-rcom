#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include "flags.h"
#include "noncanonical.c"
#include "writenoncanonical.c"
#include "aux_func.h"

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

    newtio.c_cc[VTIME]    = 2;   /* inter-character timer unused */
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


    if(argv[2] == "sender"){
        comunication_type = TRANSMITTER;
        if(argc != 4){
            printf("Must be specified a file path to transmit\n");
            exit(1);
        }
        else{
            file=fopen(argv[3],"rb"); //read file in binary mode
            if(file == NULL){
                printf("unable to open file\n");
                exit(1);
            }
        }
    }
    else if(argv[2] == "receiver"){
        comunication_type = RECEIVER;
    }
    else{
        printf("argv[2] must be sender or receiver");
        exit(1);
    }

/* llopen para estabelecer ligaçoes */
/* depende do tipo, transmitter or receiver*/


    if(comunication_type == TRANSMITTER){
        if(llopen(fd,TRANSMITTER) < 0){
            perror("could not establish connection\n");
		    exit(-1);
        }

        char buffer[MAX_SIZE];
        int name_size, file_size;
        unsigned char* control_packet_start, control_packet_end;
        int type_control_packet;
        int packet_size;

        name_size = strlen(argv[3]);
        file_size = calculate_file_size(file);

        //control_packet - inicio de transmissão
        type_control_packet = START;
        control_packet_start = create_packet(name_size,argv[3],file_size,type_control_packet,&packet_size);

        //enviar control_packet  (NOTA: a única coisa que varia no control_packet END e START é valor de C)
        llwrite(fd,control_packet_start,packet_size);

        //mandar dados
        //até ter bytes para ler, devemos ler o ficheiro, sempre em pacotes de 256 bytes
        int bytes_written = 0, bytes_read;
        int packages_sent = 1;

        while((bytes_read = fread(buffer, sizeof(char),MAX_SIZE, file)) > 0){
            //enviar os pedaços de dados dentro de data_packages
            data_packet(fd,packages_sent,bytes_read,buffer);
            packages_sent++;

            bytes_written= bytes_written + bytes_read;
        }

        fclose(file);
        

        //mandar control_package de final
        packet_size = 0;
        type_control_packet = END;
        control_packet_start = create_packet(name_size,argv[3],file_size,type_control_packet,&packet_size);
        llwrite(fd,control_packet_end,packet_size);
        
    }
    else if(comunication_type == RECEIVER){
        if(llopen(fd, RECEIVER) < 0){
		    perror("could not establish connection\n");
		    exit(-2);
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