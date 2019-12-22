#include <stdlib.h>

#define BAUDRATE B9600
#define MODEMDEVICE "/dev/ttyS0"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1
#define M_FLAG 0x7e

#define SENDER 1
#define RECEIVER 0
#define MAX_SIZE 256
#define ESCAPE 0x7d
#define ESCAPE_FLAG 0x5d
#define FLAG_ESC 0x5e
#define MAX_TRIES 3
#define ACCEPTED 1		//ACK sent correctly
#define REJECTED -1		//ACK wrong
#define _POSIX_SOURCE 1 /* POSIX compliant source */

//A field
#define M_A_SND 0x01
#define M_A_REC 0x03
#define M_A_R 0x01
#define M_C_SND 0x07
#define M_C_REC 0x03

// C field
#define UA 0x07
#define SET 0x03
#define DISC 0x0B
#define RR0 0x05
#define RR1 0x85
#define REJ0 0x01
#define REJ1 0x81

#define START 0x02
#define END 0x03
#define DATA 0x01
