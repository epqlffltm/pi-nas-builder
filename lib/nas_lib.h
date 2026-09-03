// lib/nas_lib.h

/*
 * 공통 유틸리티와 NAS 구성 핵심 로직의 선언부.
 *
 * setup_app(초기 환경 설정)과 raid*(RAID 구성)가 함께 링크해서 쓴다.
 * 두 실행 파일이 공유하는 것은 실패 처리(check_exit)와 설정 파일 중복
 * 검사(grep_config)이고, 나머지는 RAID 구성 쪽에서만 쓴다.
 */

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