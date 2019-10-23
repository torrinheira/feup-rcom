#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int calculate_size_file(FILE* file);
unsigned char* control_packet(int name_size, char* file_name,int file_size, int type_control_packet,int *packet_size);