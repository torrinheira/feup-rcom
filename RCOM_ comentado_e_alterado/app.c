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

#include "macros.h"
#include "protocolo.h"

//a função main é exclusiva e estará presente neste mesmo ficheiro
int main(int argc, char** argv){


    struct termios oldtio,newtio;
    int fd = open(argv[1], O_RDWR | O_NOCTTY );     //abertura de um file descriptor associado a um ficheiro passado como argumento na consola

    if (fd <0) {
        printf("Error opening FILE DESCRIPTOR\n");
        exit(-1);
    }

    if ( tcgetattr(fd,&oldtio) == -1) {             //salvar configurações atuais da porta
        perror("tcgetattr");
        exit(-1);
    }


    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 1;   // TIME serves as a timeout value.
    newtio.c_cc[VMIN]     = 0;   // MIN sets the number of characters to receive before the read is satisfied

    //Após a conclusão bem-sucedida, tcflush () descarta os dados gravados no objeto referido por fd (um descritor de arquivo aberto associado a um terminal)
    // mas não transmitidos ou dados recebidos mas não lidos
    //TCIOFLUSH liberta os dados recebidos mas não lidos, e os dados gravados mas não transmitidos.

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

FILE *file;                 //associado ao ficheiro que vai ser aberto para transmitir de um computador para outro
int comunication_type;      //tipo de comunicação que vai ser associado a cada computador (1 receiver e 1 sender)

//processar argumentos da consola
    if (((strcmp("/dev/ttyS0", argv[1])!=0) && (strcmp("/dev/ttyS1", argv[1])!=0) && (strcmp("/dev/ttyS2", argv[1])!=0))){

        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
	    exit(1);
	}


    if(strcmp(argv[2], "sender") == 0){
        comunication_type = TRANSMITTER;
        if(argc != 4){     //sendo transmitter terá obrigatoriamente 4 argumentos
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

        if ((file = fopen(argv[3], "rb")) == NULL)   //verificar se o ficheiro a transmitir foi aberto com sucesso
            printf("failed to open file\n");
        if (llopen(fd, comunication_type) == -1)    //estabelece comunicação e verifica-la, entre as duas máquinas
            printf("failed communication with receiver\n");

        fflush(NULL);

        char buffer[MAX_SIZE];
        char *data_packet;
        char *data_packet_stuffed;
        char *control_packet_stuffed;
        int size;
        short sequence_number = 0;
        char *control = control_frame(argv[3], file, 1, &size); //constrói pacote de controlo START
        control_packet_stuffed = stuffing(control, &size);      //dá stuffing do controlo

        if (llwrite(fd, control_packet_stuffed, size) == -1) //envia controlo
            exit(1);

        printf("A enviar...\n");

        while ((size = fread(buffer, sizeof(char), MAX_SIZE, file)) > 0){   //Lê fragmento do ficheiro e sai do ciclo quando não houver mais nada para ler


            data_packet = build_data_packet(sequence_number++, &size, buffer);              //constrói pacote de dados
            data_packet_stuffed = stuffing(data_packet, &size);                             //BCC2 e stuffing

            if (llwrite(fd, data_packet_stuffed, size) == -1)
                exit(1);

            printf("um pacote de dados enviado com sucesso\n");
        }

        control = control_frame(argv[3], file, 0, &size);       //termina a transferência de dados e é enviado um pacote de controlo END
        control_packet_stuffed = stuffing(control, &size);

        if (llwrite(fd, control_packet_stuffed, size) == -1)    //envia pacote de fim
            printf("failed to send END frame\n");

    }


    else if(comunication_type == RECEIVER){

        // establishes connection
        if(llopen(fd, RECEIVER) < 0){                       //abre comunicação como Receiver
		    perror("could not establish connection\n");
		    exit(-2);
	    }


        // receives the control package and saves the file name and its size
        int control_flag = 1;
        int length;
        char control_p[500];
        char* file_name;
        int file_size = 0;

        char stuffed_info[500];
        char* buffer;
        char* destuffed;

        while(control_flag){

            length = llread(fd, control_p);         //nº de bytes lidos

            file_name = read_control(control_p, &file_size); //lê informação do pacote de controlo enviado pelo sender(nome do ficheiro e o tamanho do mesmo)
            if(file_name != NULL){
					printf("File Name: %s\nFile Size: %d\n", file_name, file_size); //mostra a informação recebida
					send_RR(fd);                                                     //envia um trama de supervisão representativo do sucesso da operação
					file=fopen(file_name, "wb");                                     //aberto um ficheiro com o nome obtido no modo write binary
					control_flag = 0;                                                //condição para saída do ciclo
				}
				else{
                    printf("impossible to read file name!\n");
					send_REJ(fd);
                }

        }


        //parser do pacote de dados
        printf("receiving ...\n");
        while( (length = llread(fd, stuffed_info) )> 0){

			if(stuffed_info[0]==END){                                                 //reconheceu o pacote de terminação e sai do ciclo (no break)

				buffer = verify_bcc2(stuffed_info, &length);                          // verifica o sucesso ou nao da comparação do BCC2 e manda RR ou REJ consoante
				if(buffer == NULL)                                                    // resultado da função
					send_REJ(fd);
				else{
					send_RR(fd);
					break;
				}
			}
			else{
				destuffed = verify_bcc2(stuffed_info, &length);								    //Faz destuff e verifica o BCC2

				if(destuffed == NULL)
						send_REJ(fd);
				else{
					buffer = rem_data_packet(destuffed, &length);							//Remove Header do pacote de dados
					send_RR(fd);
					fwrite(buffer,1,length,file);
				}

			}

		}



    }
    else{       //se for passado um tipo diferente de comunicação no terminal
        printf("unrecognized type of communication\n");
        exit(1);
    }






/*------------------------------------------------------------ TERMINAÇÃO DA APLICAÇÃO ----------------------------------------------------------------------------------*/


    llclose(fd, comunication_type);   

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {   //repõe configurações da porta
        perror("tcsetattr");
        exit(-1);
    }

    close(fd); //fecho da porta de série
    return 0;
}
