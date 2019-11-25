#include "auxiliar_func.h"
#include "macros.h"

int main(int argc, char ** argv){

    struct hostent *h;
    int	sockfd;
	struct	sockaddr_in server_addr;

    char user[SIZE];
    char password[SIZE];
    char host[SIZE];
    char url_path[SIZE];

    if(argc != 2){
        perror("Arguments are not correct");   
    }
    else{
        
    }

    //
    return 0;
}