#include "protocolo.h"
#include "macros.h"


int flag = 0;
int timeout = 0;
int alternate = 0;

void timeOut()          //funçao usada para o timeout de operações
{
    timeout++;
    flag = 1;
}

//função efetuada de moda diferente caso comunication_type seja SENDER/TRANSMITTER (1) ou RECEIVER (0)
int llopen(int fd, int comunication_type){

    if (comunication_type)
        (void)signal(SIGALRM, timeOut);
    char A_address, C_address;
    char byte;
    int state = 0;  //representa um estado na máquina de estados

    if (!comunication_type)
        printf("Esperando emissor...\n");
    else
        printf("Esperando recetor...\n");
    while (timeout <= 3){

        if (comunication_type){ 
            send_SET_message(fd);
            alarm(3);
            flag = 0;
        }

        while (flag == 0 && state != 5)
        { //ficaremos no ciclo enquanto state diferente de 5 e flag for igual a 0

            read(fd, &byte, 1);             //lê da porta byte a byte

            switch (state){                 //verificamos em que estado nos encontramos e qual é a informação recebida no fd

            case 0: //M_FLAG
                if (byte == M_FLAG)
                    state = 1;
                break;
            case 1:
                if (comunication_type)
                    A_address = M_A_SND;
                else
                    A_address = M_C_REC;
                if (byte == A_address)
                    state = 2;
                else if (byte == M_FLAG)
                    state = 1;
                else
                    state = 0;
                break;

            case 2:
                if (comunication_type)
                    C_address = UA;
                else
                    C_address = SET;
                if (byte == C_address)
                    state = 3;
                else if (byte == M_FLAG)
                    state = 1;
                else
                    state = 0;
                break;
            case 3:
                if (byte == (A_address ^ C_address))
                    state = 4;
                else
                    state = 0;
                break;
            case 4:
                if (byte == M_FLAG){
                    state = 5;
                    if (!comunication_type) //receiver recebeu informação corretamente e envia resposta UA
                        send_UA_message(fd, comunication_type);
                    printf("Ligação Estabelecida\n");
                    return 1;
                }
                else
                    state = 0;
                break;
            }
        }
    }

    printf("TIMEOUT - Ligação Não Estabelecida...\n");
    return -1;
}

int llwrite(int fd, char *buffer, int length){

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

    //ESPERAR PELA CONFIRMAÇÃO DE SUCESSO NA MÁQUINA OPOSTA
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
    printf("TIMEOUT\n");
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

    //type 
    //if 1 -> SENDER
    //if 0 -> RECEIVER

    char address;
    int state = 0;

    char current;
    int receiveUA = 0;

    if (type == 1) // this means we are dealing with the sender
        send_DISC_message(fd, type);

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
                        send_DISC_message(fd, type);

                        state = 0;
                        receiveUA = 1;
                    }
                    else if (type == 1){
                        send_UA_message(fd, type); // TO DO

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
void send_DISC_message(int fd, int type){

    char disc_message[5];
    char address;

    disc_message[0] = M_FLAG;
    if (type == 1){                  // sender
        address = M_A_REC;
        disc_message[1] = address;
    }else{                           // receiver
        address = M_A_R;
        disc_message[1] = address;
    }

    disc_message[2] = DISC;
    disc_message[3] = address ^ DISC;
    disc_message[4] = M_FLAG;

    write(fd, disc_message, 5);

    fflush(NULL);
}

// sends UA
void send_UA_message(int fd, int type){

    char UA_message[5];
    char address;

    UA_message[0] = M_FLAG;
    if (type == 1){
        address = M_A_REC;
        UA_message[1] = address;
    }else{
        address = M_A_R;
        UA_message[1] = address;
    }

    UA_message[2] = UA;
    UA_message[3] = address ^ UA;
    UA_message[4] = M_FLAG;

    write(fd, UA_message, 5);
    fflush(NULL);
}

// Sends SET
void send_SET_message(int fd){

    char message[5];

    message[0] = M_FLAG;
    message[1] = M_A_REC;
    message[2] = SET;
    message[3] = message[1] ^ message[2];
    message[4] = M_FLAG;

    write(fd, message, 5);
    fflush(NULL);
}

// Sends REJ
void send_REJ_message(int fd){

    unsigned char c_char;

    if (alternate == 0)
        c_char = REJ1;
    else if (alternate == 1)
        c_char = REJ0;

    unsigned char message[5];

    message[0] = M_FLAG;
    message[1] = M_A_REC;
    message[2] = c_char;
    message[3] = M_A_REC ^ c_char;
    message[4] = M_FLAG;

    write(fd, message, 5);
    fflush(NULL);
}

// Sends RR
void send_RR_message(int fd){

    unsigned char c_char;
    if (alternate == 0){
        alternate = 1;
        c_char = RR1;
    }else if (alternate == 1){
        alternate = 0;
        c_char = RR0;
    }

    unsigned char message[5];

    message[0] = M_FLAG;
    message[1] = M_A_REC; //Resposta do Recetor -> 0x03
    message[2] = c_char;
    message[3] = M_A_REC ^ c_char;
    message[4] = M_FLAG;

    write(fd, message, 5);
    fflush(NULL);
}

char *stuffer(char *to_stuff, int *size){


    char *auxiliar;
    char *stuffed;
    char c_bcc = 0x00;

    stuffed = (char *)malloc(*size);
    auxiliar = (char *)malloc(*size);

    int i = 0;
    int k = 0;

    //calculates bcc2's char
    for (; i < *size; i++, k++){
        c_bcc ^= to_stuff[i];           //calcular bcc
        auxiliar[i] = to_stuff[i];      //passa para o array auxiliar o que é passado para a função
    }

    auxiliar = (char *)realloc(auxiliar, (*size) + 1);      //auxiliar realoca memória para lhe ser passado o bcc
    auxiliar[*size] = c_bcc;

    i = 0;
    k = 0;

    //stuffs - from auxiliar to stuffed
    for (; i < *size + 1; i++, k++){

        if (auxiliar[i] == M_FLAG){ //replacing flag
            stuffed[k] = 0x7d;
            stuffed[1 + k] = 0x5e;
            k++;
        }else if (auxiliar[i] == 0x7d){ //replacing flag replacer
            stuffed[k] = 0x7d;
            stuffed[1 + k] = 0x5d;
            k++;
        }else
            stuffed[k] = auxiliar[i];
    }

    //updating stuffed size
    *size = k;

    return stuffed;
}

char *destuffer(char *to_destuff, int *size){

    char *destuffed = (char *)malloc(*size);
    int updated_size = 0;
    int i;
    for (i = 0; i < *size; i++){
        updated_size++;

        if (to_destuff[i] == 0x7d){

            if (to_destuff[i + 1] == 0x5e){

                destuffed[updated_size - 1] = M_FLAG;
                i++;
            }else if (to_destuff[i + 1] == 0x5d){

                destuffed[updated_size - 1] = 0x7d;
                i++;
            }
        }else
            destuffed[updated_size - 1] = to_destuff[i];
    }

    *size = updated_size;

    return destuffed;
}

char *check_bcc2(char *c_message, int *size){

    char *destuffed_msg = destuffer(c_message, size);

    char c_bcc2 = 0x00;

    //calculates bcc2
    for (int i = 0; i < *size - 1; i++)
        c_bcc2 ^= destuffed_msg[i];

    //checks bcc2
    if (c_bcc2 != destuffed_msg[*size - 1]){
        *size = -1;
        return NULL;
    }

    *size = *size - 1;
    char *processed_message = (char *)malloc(*size);

    for (int i = 0; i < *size; i++)
        processed_message[i] = destuffed_msg[i];

    //returns message destuff and without bcc2 if correctky received
    return processed_message;
}


//parametros importantes a passar para que o reader tenha informação: nome do ficheiro e tamanho do mesmo
//passado como argumento à função : nome do ficheiro, File* associado, se o pacote de controlo é final ou inical.
char *assemble_c_frame(char *name, FILE *file, int start, int *size){

   
    int file_size = SizeOfFile(file); //tamanho do ficheiro a transmitir
    int name_size = strlen(name); //tamanho do nome do ficheiro
    if (start)
        printf("\nFile size = %d bytes\n\n", file_size);

    int i = 0;
    char fileBuffer[50];
    
    sprintf(fileBuffer, "%d", file_size); //envia saída formatada para uma string fileBuffer.


    *size = 5 + name_size + strlen(fileBuffer);
    char *c_msg = 
    malloc(*size); //aloca memória necessária para o control frame

    if (start)
        c_msg[i] = START;
    else
        c_msg[i] = END;

    i++;
    c_msg[i] = 0x00;
    i++;
    c_msg[i] = (char)strlen(fileBuffer);
    i++;

    for (; i < 3 + strlen(fileBuffer); i++)
        c_msg[i] = fileBuffer[i - 3];

    c_msg[i] = 0x01;
    i++;
    c_msg[i] = (char)name_size;
    i++;

    int final_pos = name_size + i;

    for (int s = i; i < final_pos; i++)
        c_msg[i] = name[i - s];

    return c_msg;
}

char* build_data_packet(int packages_sent, int *length, char* buffer){

    int size = *length + 4;

    unsigned char* data_package =( char*) malloc(size);

    data_package[0] = 0x00;
    data_package[1] = (char) packages_sent;
    data_package[2] = (char)(*length) / 256;
    data_package[3] = (char)(*length) % 256;

    for(size_t i = 0 ; i < *length ; i++ )
		data_package[i+4] = buffer[i];

    //atualiza o tamanho e retorna o packet com os dados
    *length = *length + 4;
    return data_package;
}


char* rem_data_packet(char* buffer, int* size){

    int length = 2 * (*size);
    char* tmp = malloc(length);

    for (int i = 0; i < *size - 4; i++)
        tmp[i] = buffer[i + 4];

    *size = *size - 4;
    return tmp;

}


char *read_control(char *control, int *file_size){

    if (control[0] != START)
        return NULL;

    int pos = 4 + control[2];
    int filename_size = control[pos];

    char *buffer = malloc(100);

    char *size = malloc(control[2]);
    int i;

    for (i = 0; i < filename_size; i++){        //ir buscar o nome do ficheiro
        buffer[i] = control[pos + 1 + i];
    }

    for (i = 0; i < control[2]; i++)             //buscar o tamanho do ficheiro
        size[i] = control[i + 3];

    *file_size = atoi(size);
    return buffer;
}

// function found @stackoverflow
int SizeOfFile(FILE *file){

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
