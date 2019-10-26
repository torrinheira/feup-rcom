#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "macros.h"

/**
 * @brief
 *
 * @param fd
 * @param type specefies sender or receiver
 * @return int fd or -1 if failed to open
 */
int llopen(int fd, int type);

int llwrite(int fd, char *buffer, int length);

int llread(int fd, char *buffer);

int llclose(int fd, int type);

void timeOut();

void send_UA_message(int fd, int type);

void send_DISC_message(int fd, int type);

void send_SET_message(int fd);

void send_RR_message(int fd);

void send_REJ_message(int fd);

char* stuffer(char* to_stuff, int* size);

char* destuffer(char* to_destuff, int* size);

char* check_bcc2(char* c_message, int *size);

char *assemble_c_frame(char *name, FILE *file, int start, int *size);

char *build_data_packet(int packages_sent, int *size, char *buffer);

char* rem_data_packet(char* buffer, int* size);

char *read_control(char *control_p, int *file_size);

int SizeOfFile(FILE *file);
