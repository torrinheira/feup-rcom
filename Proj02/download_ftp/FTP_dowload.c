#include "auxiliar_func.h"
#include "macros.h"

int main(int argc, char ** argv){

    if(argc != 2){
        perror("Arguments are not correct");   
    }
   

   //==================================================== parse arguments ================================================//

    int url_size = strlen(argv[1]);
    int new_size = url_size - 7;
    int name_size = 0;
    int password_size = 0;
    int host_size = 0;
    int url_path_size = 0;
    
    if(new_size <= 8){
        perror("Error: Check url... Something went wrong");
    }

    char argv1[new_size];

    int j = 7;
    for(size_t i = 0 ; i <= new_size ; i++){
        argv1[i] = (argv[1])[j];
        j++;
    }

    //argv1 é uma string igual à passada no argv1 mas sem a parte do ftp://

    //obtem nome do utilizador
    while(argv1[name_size] != ':'){
        name_size++;
    }

    char name[name_size];

    for(size_t i = 0 ; i < name_size ; i++){
        name[i] = argv1[i];
    }

    //obter password do utilizador
    
    printf("%s\n", name);
    printf("%s\n",argv1);

    //=============================================================================================//
    
    return 0;
}