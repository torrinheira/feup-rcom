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
char *control_frame(char *filename, FILE *file, int start, int *frame_size){

    int file_name_size = strlen(filename);
    int file_size = calculate_size_file(file); //Get file size
    if (start)
        printf("\nFile size = %d bytes\n\n", file_size);
    int i = 0;
    char file_size_in_string[30];
    sprintf(file_size_in_string, "%d", file_size);

    *frame_size = 5 + file_name_size + strlen(file_size_in_string);
    char *control_frame = malloc(*frame_size);

    if (start)
        control_frame[i++] = START;
    else
        control_frame[i++] = END;

    control_frame[i++] = 0x00;
    control_frame[i++] = (char)strlen(file_size_in_string);

    for (; i < strlen(file_size_in_string) + 3; i++){

        control_frame[i] = file_size_in_string[i - 3];
    }

    control_frame[i++] = 0x01;
    control_frame[i++] = (char)file_name_size;

    int j;
    for (j = i; i < file_name_size + j; i++)
    {

        control_frame[i] = filename[i - j];
    }

    return control_frame;
}

char* data_packet(int packages_sent, int *length, char* buffer){

    int size = *length + 4;
    unsigned char* data_package =( char*) malloc(size);
    data_package[0] = 0x00;
    data_package[1] = (char) packages_sent;
    data_package[2] = (char) (*length) / 256;
    data_package[3] = (char) (*length) % 256;

    for(size_t i = 0 ; i < *length ; i++ ){
		data_package[i+4] = buffer[i];
	}

    // llwrite(fd,data_package,length);
    // free(data_package);
    *length = *length + 4;
    return data_package;
}


char* rem_data_packet(char* buffer, int* length){

    int size = 2 * (*length);
    char* tmp = malloc(size);

    for(int i = 0; i < *length - 4; i++){
        tmp[i] = buffer[i + 4];
    }

    *length = *length - 4;
    return tmp;

}


char *get_info(char *control, int *file_size)
{

    if (control[0] != START)
        return NULL;
    int pos = 4 + control[2];
    int filename_size = control[4 + control[2]];

    char *buffer = malloc(100);

    char *size = malloc(control[2]);
    int i;

    for (i = 0; i < filename_size; i++)
    {
        buffer[i] = control[pos + 1 + i];
    }

    for (i = 0; i < control[2]; i++)
        size[i] = control[i + 3];

    *file_size = atoi(size);
    return buffer;
}

int getFileSize(FILE *file){

    printf("desobrimos?\n");
    int currentPosition = ftell(file);

    printf("sera aqui?\n");
    if (fseek(file, 0, SEEK_END) == -1)
    {
        printf("ERROR: Could not get file size.\n");
        return -1;
    }

    int size = ftell(file);

    fseek(file, 0, currentPosition);

    return size;
}