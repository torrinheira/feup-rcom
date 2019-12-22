#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>



#include "macros.h"

void read_answer(int socket, char *host_answer);
int parseResponse(char* response);
void create_file(int sockfd_file_transfer, char* path_file);
void readResponse(int sockfd, char *response);
