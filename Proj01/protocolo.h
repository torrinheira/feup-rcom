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

void send_UA(int fd, int type);

void send_DISC(int fd, int type);

void send_SET(int fd);

void send_RR(int fd);

void send_REJ(int fd);

char* stuffing(char* data_package, int* length);

char* destuffing(char* msg, int* length);

char* verify_bcc2(char* control_message, int *length);

int calculate_size_file(FILE* file);

char *control_frame(char *filename, FILE *file, int start, int *frame_size);

char *data_packet(int packages_sent, int *length, char *buffer);

char* rem_data_packet(char* buffer, int* length);

char *read_control(char *control_p, int *file_size);

int getFileSize(FILE *file);
