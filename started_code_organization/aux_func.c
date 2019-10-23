#include "aux_func.h"
#include "flags.h"

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
    control_frame[index++] = 0x00;
    control_frame[index++] = name_size;

    //ciclo para preencher V1
    for(size_t j = 0; j < name_size ;j++){
        control_frame[index++] = file_name[j];
    }

    control_frame[index++] = 0x00;
    control_frame[index++] = name_size;

    //ciclo para preencher V1
    for(size_t j = 0; j < strlen(fileBuffer) ;j++){
        control_frame[index++] = fileBuffer[j];
    }

    return control_frame;
}