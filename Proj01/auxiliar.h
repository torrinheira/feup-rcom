#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int calculate_size_file(FILE* file);
char *control_frame(char *filename, FILE *file, int start, int *frame_size);

char *data_packet(int packages_sent, int *length, char *buffer);
char* rem_data_packet(char* buffer, int* length);

char *get_info(char *control_p, int *file_size);
int getFileSize(FILE *file);
