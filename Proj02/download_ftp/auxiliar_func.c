#include "auxiliar_func.h"

void read_answer(int socket, char *host_answer){

    bool stop = false;
    char response;
    int estado = 0;
    int i = 0;

    while(!stop){
        //ler do socket um char de cada vez
        read(socket, &response, 1);
        //imprime na consola a resposta do user
		printf("%c", response);
        
        switch(estado){
            case 0:
                if(i == 3){
                    estado = 1;
                }
                else{
                    host_answer[i] = response;
                    i++;
                }

                break;
            case 1:
                if(response == '\n'){
                    stop = true;
                }
                break;
        }
    }
}

int parseResponse(char* response){
    char mostSig[5];
	memset(mostSig, 0, 5);
	char lessSig[4];
	memset(lessSig, 0, 5);

    int i = 0;
    int ms = 0;
    int ls = 0;
    int state = 0;
    int number_comma = 0;

    while(i < strlen(response)){
        if(state == 0){
            if(response[i] == '('){
                state = 1;
                i++;
            }
            else{
                i++;
            }
        }
        else if(state == 1){
            if(number_comma < 4){
                if(response[i] == ','){
                    number_comma++;
                }
                i++;
            }
            else if(number_comma == 4){
                if(response[i] == ','){
                    number_comma++;
                }
                else{
                    mostSig[ms] = response[i];
                    ms++;
                }
                i++;
            }
            else if(number_comma == 5){
                if(response[i] == ')'){
                    state = 2;;
                }
                else{
                    lessSig[ls] = response[i];
                    ls++;
                }
                i++;
            }

        }
        else{
            i++;
        }

    }

   
    int mostSignificant = atoi(mostSig);
	int lessSignificant = atoi(lessSig);
	return (mostSignificant * 256 + lessSignificant);
}

void create_file(int sockfd_file_transfer, char* path_file){
	FILE *file = fopen((char *)path_file, "wb+");

    
    char bufSocket[1024];
 	int bytes;
    int counter = 0;

    //nao faz nenhuma vez este ciclo
	printf("> Starting download!\n");

 	while ((bytes = read(sockfd_file_transfer, bufSocket, 1024))>0) {
    	bytes = fwrite(bufSocket, bytes, 1, file);   
    }

    fclose(file);
	printf("\n");
	printf("> Done!\n");

}
//150
//226