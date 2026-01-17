#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

void check_exit(int status, const char *message) 
{
    if (status != 0) 
    {
        fprintf(stderr, "\n오류 발생: %s (코드: %d)\n", message, status);
        exit(EXIT_FAILURE);
    }
}

int grep_config(const char *file_name, const char *search) 
{
    FILE *file = fopen(file_name, "r");
    if (file == NULL) return 0;
    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), file)) 
    {
        if(strstr(line, search) != NULL) 
        { 
            found = 1; break; 
        }
    }
    fclose(file);
    return found;
}

long get_total_memory_size_kb(void) 
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) return -1;
    char label[64]; long mem_kb = 0;
    if (fscanf(fp, "%s %ld", label, &mem_kb) != 2) 
    { 
        fclose(fp); return -1;
    }
    fclose(fp);
    return mem_kb;
}