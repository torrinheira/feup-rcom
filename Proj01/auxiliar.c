#include "auxiliar.h"
#include "flags.h"
#include "protocolo.h"

//function found @stackoverflow
int calculate_size_file(FILE* file){

    int size = 0;
    int value;

    // saving current position
	int currentPosition = ftell(file);

	// seeking end of file
    value = fseek(file, 0, SEEK_END);
    if(value == -1){
        printf("impossible to get file size to transfer.\n");
		return -1;  
    }

	// saving file size
	size = ftell(file);

	// seeking to the previously saved position
	fseek(file, 0, currentPosition);

    return size;
}

//parametros importantes a passar para que o reader tenha informação: nome do ficheiro e tamanho a do mesmo
unsigned char* control_packet(int name_size, char* file_name, int file_size, int type_control_packet,int *packet_size){

    char fileBuffer[100];
	sprintf(fileBuffer, "%d", file_size);
    
    *packet_size = 5 + strlen(fileBuffer) + name_size; //5 é o C, T1, L1, T2 E L2, o filze_size e name_size será importante para o V
    int index = 0;

    unsigned char control_frame[*packet_size];


    if(type_control_packet == START){
        control_frame[index] = START;
    }
    else if(type_control_packet == END){
        control_frame[index] = END;
    }

    //preenche T1 e L1
    control_frame[index++] = 0x01;
    control_frame[index++] = name_size;

    //ciclo para preencher V1
    for(size_t j = 0; j < name_size ;j++){
        control_frame[index++] = file_name[j];
    }

    control_frame[index++] = 0x00;
    control_frame[index++] = file_size;

    //ciclo para preencher V1
    for(size_t j = 0; j < strlen(fileBuffer) ;j++){
        control_frame[index++] = fileBuffer[j];
    }

    return control_frame;
}



char* data_packet(int fd, int packages_sent, int length, char* buffer){

    int size = length + 4;
    unsigned char* data_package =( char*) malloc(size);
    data_package[0] = 0x00;
    data_package[1] = (char) packages_sent;
    data_package[2] = (char) packages_sent / 256;
    data_package[3] = (char) packages_sent % 256;

    for(size_t i = 0 ; i < length ; i++ ){
		data_package[i+4] = buffer[i];
	}

    // llwrite(fd,data_package,length);
    // free(data_package);
    return data_package;
}

char* get_info(char* control_p, int* file_size){

    if(control_p[0] == END){
        printf("END packet received\n");
        return NULL;
        // TO DO
    }
    else if(control_p[0] == START){        // received filename and file size

        int filename_size = control_p[2];
        char *filename = malloc(500);
        for(int i = 0 ; i < filename_size ; i++ ){
		    filename[i] = control_p[3 + i];
	    }

        int size_pos = 4+control_p[2];
        char* size = malloc(control_p[2]);
        for(int i = 0; i < control_p[size_pos]; i++){
            size[i] = control_p[size_pos + 1 + i];
        }

        *file_size = atoi(size);
        return filename;


    }


}
