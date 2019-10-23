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
#include "auxiliar.h"

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS0"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define M_FLAG 0x7e
#define M_A_SND 0x01
#define M_A_REC 0x03
#define M_C_SND 0x07
#define M_C_REC 0x03
#define M_A_R 0x01
#define DISC 0x0B
#define UA 0x07
#define SET 0x03
#define TRANSMITTER 0
#define RECEIVER 1
#define MAX_SIZE 255
#define ESCAPE 0x7d
#define ESCAPE_FLAG 0x5d
#define FLAG_ESC 0x5e
#define MAX_TRIES 3
#define ACCEPTED 1      //ACK sent correctly
#define REJECTED -1     //ACK wrong
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81

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

/**
 * @brief increases counter and raises flah
 * 
 */
void timeOut();

void send_UA(int fd, int type);

void send_DISC(int fd, int type);

void send_SET(int fd);

void send_RR(int fd);

char* stuffing(char* payload, int* length);

char* destuffing(char* msg, int* length);

char* verify_bcc2(char* control_message, int *length);

