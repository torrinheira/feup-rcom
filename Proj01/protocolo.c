#include "protocolo.h"
#include "macros.h"


int flag = 0;
int timeout = 0;
int alternate = 0;

void timeOut()
{
    timeout++;
    flag = 1;
}


int llopen(int fd, int comunication_type){

    if (comunication_type)//sender
        (void)signal(SIGALRM, timeOut);
    char address, address2;
    char byte;
    int state = 0;

    if (!comunication_type)
        printf("Esperando emissor...\n");
    else
        printf("Esperando recetor...\n");
    while (timeout <= 3){

        if (comunication_type){ //Sender
            send_SET(fd);
            alarm(3);
            flag = 0;
        }

        while (state != 5 && flag == 0){

            read(fd, &byte, 1);

            switch (state){

            case 0: //M_FLAG
                if (byte == M_FLAG)
                    state = 1;
                break;
            case 1:
                if (comunication_type)
                    address = M_A_SND;
                else
                    address = M_C_REC;
                if (byte == address)
                    state = 2;
                else if (byte == M_FLAG)
                    state = 1;
                else
                    state = 0;
                break;

            case 2:
                if (comunication_type)
                    address2 = UA;
                else
                    address2 = SET;
                if (byte == address2)
                    state = 3;
                else if (byte == M_FLAG)
                    state = 1;
                else
                    state = 0;
                break;
            case 3:
                if (byte == (address ^ address2))
                    state = 4;
                else
                    state = 0;
                break;
            case 4:
                if (byte == M_FLAG){
                    state = 5;
                    if (!comunication_type) //receiver
                        send_UA(fd, comunication_type);
                    printf("Ligação Estabelecida\n");
                    return 1;
                }
                else
                    state = 0;
                break;
            }
        }
    }

    printf("timeout - Ligação Não Estabelecida\n");
    return -1;
}

int llwrite(int fd, char *buffer, int length)
{

    fflush(NULL);
    timeout = 0;
    char *trama = malloc((length + 5) * sizeof(char));
    unsigned char controlo;
    unsigned char byte;
    int i, written, state = 0;

    if (alternate == 0){
        alternate = 1;
        controlo = RR0;
    }
    else if (alternate == 1){
        alternate = 0;
        controlo = RR1;
    }

    //Prepara bytes iniciais
    trama[0] = M_FLAG;
    trama[1] = M_A_REC;
    trama[2] = controlo;
    trama[3] = trama[1] ^ trama[2];
    trama[length + 4] = M_FLAG;


    for (i = 4; i < length + 4; i++)
        trama[i] = buffer[i - 4];

    if(controlo == RR0) // expects the opposite bit on RR
        controlo = RR1;
    else controlo = RR0;

    //ESPERAR PELO ACK
    (void)signal(SIGALRM, timeOut);
    timeout = 0;
    while (timeout <= 3){

        written = write(fd, trama, length + 5);
        written = written - 5;
        alarm(3);
        flag = 0;
        state = 0; //ints representing states


        while (state != 5 && flag == 0){
            printf("state %d\n", state);

            read(fd, &byte, 1);
            printf("%x\n",byte);

            switch (state){
                case 0:
                    if (byte == M_FLAG)
                        state = 1;
                    break;
                case 1:
                    if (byte == M_A_REC)
                        state = 2;
                    else if (byte == M_FLAG)
                        state = 1;
                    else
                        state = 0;
                    break;

                case 2:

                    if (byte == RR0 || byte == RR1)
                        state = 3;
                    else if (byte == REJ0 || byte == REJ1){
                        state = 0;
                        flag = 1;
                    }
                    else if (byte == M_FLAG){
                        state = 1;

                    }else
                        state = 0;
                    break;
                case 3:
                    if (byte == (M_A_REC ^ controlo))
                        state = 4;
                    else
                        state = 0;
                    break;
                case 4:
                    if (byte == M_FLAG){
                        state = 5;
                        return written;
                    }
                    else
                        state = 0;
                    break;
            }
        }
    }
    printf("TIMEOUT");
    return -1;
}

// LLREAD
int llread(int fd, char *buffer){

    int length = 0;
    int state = 0;
    char acknowledge;

    char current;

    if (alternate == 0)
        acknowledge = RR0;
    else if (alternate == 1)
        acknowledge = RR1;

    while (state != 5){

        read(fd, &current, 1);

        switch (state){

        case 0:
            if (current == M_FLAG)// first byte ok !
                state = 1;
            break;
        case 1:
            if (current == M_A_REC) // second byte ok !
                state = 2;
            else if (current == M_FLAG)
                state = 1;
            else
                state = 0;
            break;
        case 2:
            if (current == acknowledge) // third byte ok !
                state = 3;
            else if (current == M_FLAG)
                state = 1;
            else
                state = 0;
            break;
        case 3:
            if (current == (M_A_REC ^ acknowledge)) // fourth byte ok !
                state = 4;
            else
                state = 0;
            break;
        case 4:
            if (current == M_FLAG) // receiving termination FLAG ends the cycle
                state = 5;
            else{ // fills buffer with the chunk of information
                buffer[length] = current;
                length++;
            }
            break;
        }
    }

    return length;
}

// LLCLOSE --> function used by both receiver and sender (type)
int llclose(int fd, int type)
{

    char address;
    int state = 0;

    char current;
    int receiveUA = 0;

    if (type == 1) // this means we are dealing with the sender
        send_DISC(fd, type);

    while (timeout <= 3 || !type){

        if (type == 0){
            alarm(3);
            flag = 0;
        }

        while (state != 5 && flag == 0){

            read(fd, &current, 1);

            switch (state){

            case 0:
                if (current == M_FLAG)
                    state = 1;
                break;
            case 1:

                if (type == 0)
                    address = M_A_REC;
                else if (type == 1)
                    address = M_A_R;

                if (current == address)
                    state = 2;
                else if (current == M_FLAG)
                    state = 1;
                else
                    state = 0;
                break;
            case 2:
                if (current == DISC)// third byte ok !
                    state = 3;
                else if (current == UA && receiveUA == 1)
                    state = 3;
                else if (current == M_FLAG)
                    state = 1;
                else
                    state = 0;
                break;
            case 3:
                if (current == (address ^ DISC))// fourth byte ok !
                    state = 4;
                else if ((receiveUA == 1) && (current == (UA ^ address)))
                    state = 4;
                else
                    state = 0;
                break;
            case 4:
                if (current == M_FLAG){
                    state = 5;
                    if (type == 0 && receiveUA == 0){
                        send_DISC(fd, type);

                        state = 0;
                        receiveUA = 1;
                    }
                    else if (type == 1){
                        send_UA(fd, type); // TO DO

                        printf("Ligação encerrada!\n");
                        return 1;
                    }
                    else if (receiveUA == 1){

                        printf("Ligação encerrada!\n");
                        return 1;
                    }
                }
                else
                    return -1;
                break;
            }
        }
    }
}

// sends DISC depending on weather its a receiver or sender
void send_DISC(int fd, int type){

    char trama[5];
    char address;

    trama[0] = M_FLAG;
    if (type == 1){ // sender
        address = M_A_REC;
        trama[1] = address;
    }
    else{ // receiver
        address = M_A_R;
        trama[1] = address;
    }
    trama[2] = DISC;
    trama[3] = address ^ DISC;
    trama[4] = M_FLAG;

    write(fd, trama, 5);
    fflush(NULL);
}

// sends UA
void send_UA(int fd, int type){

    char UA_trama[5];
    char address;

    UA_trama[0] = M_FLAG;
    if (type == 1){
        address = M_A_REC;
        UA_trama[1] = address;
    }
    else{
        address = M_A_R;
        UA_trama[1] = address;
    }
    UA_trama[2] = UA;
    UA_trama[3] = address ^ UA;
    UA_trama[4] = M_FLAG;

    write(fd, UA_trama, 5);
    fflush(NULL);
}

// Sends SET
void send_SET(int fd)
{

    char trama[5];

    trama[0] = M_FLAG;
    trama[1] = M_A_REC;
    trama[2] = SET;
    trama[3] = M_A_REC ^ SET;
    trama[4] = M_FLAG;

    write(fd, trama, 5);
    fflush(NULL);
}

// Sends REJ
void send_REJ(int fd)
{

    unsigned char controlo;
    if (alternate == 0)
    {
        controlo = REJ1;
    }
    else if (alternate == 1)
    {
        controlo = REJ0;
    }

    unsigned char trama[5];
    trama[0] = M_FLAG;
    trama[1] = M_A_REC;
    trama[2] = controlo;
    trama[3] = M_A_REC ^ controlo;
    trama[4] = M_FLAG;

    write(fd, trama, 5);
    fflush(NULL);
}

// Sends RR
void send_RR(int fd)
{

    unsigned char controlo;
    if (alternate == 0)
    {
        alternate = 1;
        controlo = RR1;
    }
    else if (alternate == 1)
    {
        alternate = 0;
        controlo = RR0;
    }

    unsigned char trama[5];
    trama[0] = M_FLAG;
    trama[1] = M_A_REC; //Resposta do Recetor -> 0x03
    trama[2] = controlo;
    trama[3] = M_A_REC ^ controlo;
    trama[4] = M_FLAG;

    write(fd, trama, 5);
    fflush(NULL);
}

char *stuffing(char *data_package, int *length){

    char *str;
    char *aux;
    int i;
    int j;

    str = (char *)malloc(*length);
    aux = (char *)malloc(*length);
    char BCC2 = 0x00;

    for (i = 0, j = 0; i < *length; i++, j++)
    {
        BCC2 ^= data_package[i];
        aux[i] = data_package[i];
    }

    aux = (char *)realloc(aux, (*length) + 1);
    aux[*length] = BCC2;

    for (i = 0, j = 0; i < *length + 1; i++, j++)
    {

        if (aux[i] == M_FLAG){      //replacing flag
            str[j] = 0x7d;
            str[j + 1] = 0x5e;
            j++;
        }
        else if (aux[i] == 0x7d){   //replacing flag replacer
            str[j] = 0x7d;
            str[j + 1] = 0x5d;
            j++;
        }
        else
            str[j] = aux[i];
    }

    *length = j;

    return str;
}

char *destuffing(char *msg, int *length){

    char *str = (char *)malloc(*length);
    int i;
    int updated_length = 0;

    for (i = 0; i < *length; i++)
    {
        updated_length++;

        if (msg[i] == 0x7d){
            if (msg[i + 1] == 0x5e){
                str[updated_length - 1] = M_FLAG;
                i++;
            }
            else if (msg[i + 1] == 0x5d){
                str[updated_length - 1] = 0x7d;
                i++;
            }
        }
        else
            str[updated_length - 1] = msg[i];
    }
    *length = updated_length;

    return str;
}

char *verify_bcc2(char *control_message, int *length)
{

    char *destuffed_message = destuffing(control_message, length);

    int i;
    char control_bcc2 = 0x00;
    for (i = 0; i < *length - 1; i++){
        control_bcc2 ^= destuffed_message[i];
    }

    if (control_bcc2 != destuffed_message[*length - 1]){
        *length = -1;
        return NULL;
    }
    *length = *length - 1;
    char *data_message = (char *)malloc(*length);
    for (i = 0; i < *length; i++){
        data_message[i] = destuffed_message[i];
    }

    return data_message;
}


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
        control_frame[i] = START;
    else
        control_frame[i] = END;

    i++;
    control_frame[i] = 0x00;
    i++;
    control_frame[i] = (char)strlen(file_size_in_string);
    i++;

    for (; i < strlen(file_size_in_string) + 3; i++){

        control_frame[i] = file_size_in_string[i - 3];
    }

    control_frame[i] = 0x01;
    i++;
    control_frame[i] = (char)file_name_size;
    i++;

    int j;
    for (j = i; i < file_name_size + j; i++){

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

    for(size_t i = 0 ; i < *length ; i++ )
		data_package[i+4] = buffer[i];


    // llwrite(fd,data_package,length);
    // free(data_package);
    *length = *length + 4;
    return data_package;
}


char* rem_data_packet(char* buffer, int* length){

    int size = 2 * (*length);
    char* tmp = malloc(size);

    for(int i = 0; i < *length - 4; i++)
        tmp[i] = buffer[i + 4];

    *length = *length - 4;
    return tmp;

}


char *read_control(char *control, int *file_size){

    if (control[0] != START)
        return NULL;

    int pos = 4 + control[2];
    int filename_size = control[4 + control[2]];

    char *buffer = malloc(100);

    char *size = malloc(control[2]);
    int i;

    for (i = 0; i < filename_size; i++){
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
    if (fseek(file, 0, SEEK_END) == -1){
        printf("ERROR: Could not get file size.\n");
        return -1;
    }

    int size = ftell(file);

    fseek(file, 0, currentPosition);

    return size;
}
