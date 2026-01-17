#ifndef UTILS_H
#define UTILS_H

void check_exit(int status, const char *message);
int grep_config(const char *file_name, const char *search);
long get_total_memory_size_kb(void);

#endif