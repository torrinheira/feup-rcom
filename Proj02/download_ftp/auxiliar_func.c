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
	FILE *file = fopen(path_file, "wb+");

    
    char bufSocket[1000];
 	int bytes;
    int counter = 0;

	printf("> Starting download!\n");

    while ((bytes = read(sockfd_file_transfer, bufSocket, 1000))>0) {
	        printf("> Start!\n");
            bytes = fwrite(bufSocket, bytes, 1, file);   
    }
   
    fclose(file);
	printf("\n");
	printf("> Done!\n");

}

//reads response code from the server
void readResponse(int socketfd, char *responseCode)
{
	int state = 0;
	int index = 0;
	char c;

	while (state != 3)
	{	
		read(socketfd, &c, 1);
		printf("%c", c);
		switch (state)
		{
		//waits for 3 digit number followed by ' ' or '-'
		case 0:
			if (c == ' ')
			{
				if (index != 3)
				{
					printf(" > Error receiving response code\n");
					return;
				}
				index = 0;
				state = 1;
			}
			else
			{
				if (c == '-')
				{
					state = 2;
					index=0;
				}
				else
				{
					if (isdigit(c))
					{
						responseCode[index] = c;
						index++;
					}
				}
			}
			break;
		//reads until the end of the line
		case 1:
			if (c == '\n')
			{
				state = 3;
			}
			break;
		//waits for response code in multiple line responses
		case 2:
			if (c == responseCode[index])
			{
				index++;
			}
			else
			{
				if (index == 3 && c == ' ')
				{
					state = 1;
				}
				else 
				{
				  if(index==3 && c=='-'){
					index=0;
					
				}
				}
				
			}
			break;
		}
	}
}
//150
//226