#include "auxiliar_func.h"
#include "macros.h"
#include <stdbool.h>

int main(int argc, char ** argv){

    if(argc != 2){
        printf("Arguments are not correct\n"); 
        return -2;  
    }
   

   //==================================================== parse arguments ================================================//

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

    char name[100];
    memset(name, 0, 100);

    char password[100];
    memset(password, 0, 100);

    char host[100];
    memset(host, 0, 100);

    char path_file[100];
    memset(path_file, 0, 100);

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


    printf("Name: %s\n", name);
    printf("Password: %s\n", password);
    printf("Host: %s\n", host);
    printf("URL path: %s\n",path_file);

    //=============================================================get host ip======================================================
    
    
    return 0;
    
}