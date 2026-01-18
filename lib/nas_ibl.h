#ifndef NAS_LIB_H
#define NAS_LIB_H

// 유틸리티 함수
void check_exit(int status, const char *message);
int grep_config(const char *file_name, const char *search);

// RAID 및 저장소 핵심 로직
void cleanup_disks(const char **disks, int count);
void create_raid(int level, const char **disks, int count);
void setup_samba_and_fstab(const char *username, const char *password, int level);

#endif