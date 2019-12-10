#include "auxiliar_func.h"
#include "macros.h"


int main(int argc, char ** argv){



    if(argc != 2){
        printf("Arguments are not correct\n"); 
        return -2;  
    }

   
   


    //====================================================================================================================//
    //==================================================== parse arguments ===============================================//
    //====================================================================================================================//


    int url_size = strlen(argv[1]);
    int new_size = url_size - 7;

     
    if(new_size <= 8){
        printf("Error: Check url... Something went wrong\n");
        return -1;
    }

    char argv1[new_size];

    int j = 7;
    for(size_t i = 0 ; i <= new_size ; i++){
        argv1[i] = (argv[1])[j];
        j++;
    }

    //state machine

    char name[SIZE];
    memset(name, 0, SIZE);

    char password[SIZE];
    memset(password, 0, SIZE);

    char host[SIZE];
    memset(host, 0, SIZE);

    char path_file[SIZE];
    memset(path_file, 0, SIZE);

    char file_name[SIZE];
    memset(file_name, 0, SIZE);

    

    int estado = 0;
    int i = 0;
    int indice = 0;

    bool erro = false;
    bool found_1 = false;
    bool found_2 = false;
    bool found_3 = false;
    

    
    while( i < strlen(argv1) && !erro){
        switch (estado)
        {
        case 0:
            if(argv1[i] == ':'){
                estado = 1;
                indice = 0;
                found_1 = true;
            }
            else{
                name[indice] = argv1[i];
                indice++;
            }
            break;
        
        case 1:
            if(argv1[i] == '@'){
                estado = 2;
                indice = 0;
                found_2 = true;
            }
            else{
                password[indice] = argv1[i];
                indice++;
            }
            break;
        case 2:
            if(argv1[i] == ']'){
                estado = 3;
                indice = 0;
            }
            else{
                erro = true;
            }
            break;
        case 3:
            if(argv1[i] == '/'){
                estado = 4;
                indice = 0;
                found_3 = true;
            }
            else{
                host[indice] = argv1[i];
                indice++;
            }
            break;
        
        case 4:
            path_file[indice] = argv1[i];
            indice++;
            break;
        }

        i++;
    }

    if(erro || !found_1 || !found_2 || !found_3){
        printf("Error parsing url\n");
        return -1;
    }

    //com o urlpath fazer parser ate chegar ao filename
    int n_barras = 0;
    for(int i = 0; i < strlen(path_file); i++){
        if(path_file[i] == '/'){
            n_barras++;
        }
    }

    int barras_atuais = 0;
    int indice_file = 0;
    for(int i = 0; i < strlen(path_file); i++){
        if(barras_atuais == n_barras){
            file_name[indice_file] = path_file[i];
            indice_file++;
        }

        if(path_file[i] == '/'){
            barras_atuais++;
        }

    }


    printf("Name: %s\n", name);
    printf("Password: %s\n", password);
    printf("Host: %s\n", host);
    printf("URL path: %s\n",path_file);
    printf("Filename: %s\n",file_name);





    //===============================================================================================================================//    
    //============================================================ get host ip ======================================================//
    //===============================================================================================================================//    


    //check given code to understand
    struct hostent *h;
    

    if((h=gethostbyname(host)) == NULL){  
        herror("gethostbyname");
        exit(1);
    }

    char* ip = inet_ntoa(*((struct in_addr *) h->h_addr));
    printf("IP Address : %s\n", ip);





    //===============================================================================================================================//    
    //===================================================== FTP client process ======================================================//
    //===============================================================================================================================//  
    

    //check clientTCP.c from the given code (lot of code taken from there)
    int	sockfd; //um socket para estabelecer ligação
    int sockfd_file_transfer; //um socket para download do ficherio
	struct	sockaddr_in server_addr;
	struct	sockaddr_in server_addr_file_transfer;


    /*server address handling*/
	bzero((char*)&server_addr,sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);	/*32 bit Internet address network byte ordered*/
	server_addr.sin_port = htons(SERVER_PORT);		/*server TCP port must be network byte ordered */

    /*open an TCP socket*/
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    		perror("socket()");
        	exit(0);
    	}
	/*connect to the server*/
    	if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        	perror("connect()");
		    exit(0);
	}

    
	char responseCode[3];
   readResponse(sockfd, responseCode); 
	if (responseCode[0] == '2')
	{										 
		printf("> Connection Estabilished\n"); 
	}

   

    printf("> Sending username \n");
   
    char response[SIZE];
    dprintf(sockfd, "user %s\r\n", name);
    read(sockfd, response, SIZE);

    if(strncmp(response, "331", 3) == 0){//user certo,  mandar pass
        printf("> Sending password \n");

        char response2[SIZE];
        dprintf(sockfd, "pass %s\r\n", password);
        read(sockfd, response2, SIZE);


        if(strncmp(response2, "230", 3) == 0){// se pass certa dowload ficheiro(entrar em modo passivo e abrir 2º socket)
            //entrar em modo passivo e fazer download do ficheiro
	        dprintf(sockfd, "pasv\r\n");//printf para um filedescriptor
            printf("> Entered passive mode\n");

            char response3[SIZE];
            read(sockfd, response3, SIZE);
            printf("%s", response3);

            int port_to_download = parseResponse(response3);
            printf("Port: %d\n", port_to_download);

            /*server address handling*/
            bzero((char*)&server_addr_file_transfer,sizeof(server_addr_file_transfer));
            server_addr_file_transfer.sin_family = AF_INET;
            server_addr_file_transfer.sin_addr.s_addr = inet_addr(ip);	        /*32 bit Internet address network byte ordered*/
            server_addr_file_transfer.sin_port = htons(port_to_download);		/*server TCP port must be network byte ordered */

            /*open an TCP socket*/
            if ((sockfd_file_transfer = socket(AF_INET,SOCK_STREAM,0)) < 0) {
                    perror("socket()");
                    exit(0);
                }
            /*connect to the server*/
                if(connect(sockfd_file_transfer, (struct sockaddr *)&server_addr_file_transfer, sizeof(server_addr_file_transfer)) < 0){
                    perror("connect()");
                    exit(0);
            }

            printf("> Sending retrieve\n");
	        dprintf(sockfd, "retr %s\r\n", path_file);//printf para um filedescriptor(antigo)
            create_file(sockfd_file_transfer, file_name);
            
        }
        else if(strncmp(response2, "430", 3) == 0){
            printf("> Invalid credentials \n");
            return -1;
        }
        else{
            printf("> Error occured2 \n");
            return -1;
        }
    }
    else{
        printf("> Error occured1 \n");
        
    }

    close(sockfd_file_transfer);
	close(sockfd);

    

    return 0;
    
}

//       ftp://[anonymous:anonymous@]speedtest.tele2.net/1KB.zip

