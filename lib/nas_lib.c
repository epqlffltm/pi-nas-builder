// lib/nas_lib.c

/*
 * 공통 유틸리티와 NAS 구성 핵심 로직의 구현부.
 *
 * 이 파일의 함수들은 대부분 system()으로 외부 명령을 부른다. C로 감싼 이유는
 * 각 단계의 실패를 종료 코드로 받아 다음 단계로 넘어가지 않게 막기 위해서다.
 * mdadm --create 가 실패했는데 mkfs.ext4 가 이어서 도는 것이 가장 위험하다.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../lib/nas_lib.h"

void check_exit(int status, const char *message) 
{
  if (status != 0) 
  {
    fprintf(stderr, "\n[오류] %s (코드: %d)\n", message, status);
    exit(EXIT_FAILURE);
  }
}

/* 설정 파일에 해당 문자열이 이미 있는지 확인한다.
   setup_app 은 재실행될 수 있으므로 같은 줄을 중복으로 쓰지 않기 위해 쓴다.
   파일이 없으면 "없다"로 본다 — 호출부가 곧 새로 만들기 때문이다.
   주석 처리된 줄은 실제로 적용되지 않으므로 건너뛴다. */
int grep_config(const char *file_name, const char *search) 
{
  FILE *fp = fopen(file_name, "r");
  if (fp == NULL) return 0;

  char line[512];
  int found = 0;

  while (fgets(line, sizeof(line), fp)) 
  {
    if (line[0] == '#') continue;
    if (strstr(line, search)) 
    {
      found = 1;
      break;
    }
  }

  fclose(fp);
  return found;
}

void cleanup_disks(const char **disks, int count) 
{
  for (int i = 0; i < count; i++) 
  {
    char cmd[256];
    sprintf(cmd, "mdadm --zero-superblock %s 2>/dev/null", disks[i]);
    system(cmd);
    sprintf(cmd, "wipefs -a %s && sgdisk --zap-all %s", disks[i], disks[i]);
    system(cmd);
  }
}

void create_raid(int level, const char **disks, int count) 
{
  char disk_list[512] = "";
  for (int i = 0; i < count; i++)
  {
  strcat(disk_list, disks[i]);
  strcat(disk_list, " ");
  }
  char cmd[1024];
  sprintf(cmd, "mdadm --create --verbose /dev/md0 --level=%d --raid-devices=%d %s --run", level, count, disk_list);
  check_exit(system(cmd), "RAID 생성 실패");
}

void setup_samba_and_fstab(const char *username, const char *password, int level) 
{
  // Samba 계정 등록
  char smb_cmd[512];
  sprintf(smb_cmd, "(echo \"%s\"; echo \"%s\") | smbpasswd -s -a %s", password, password, username);
  system(smb_cmd);

  // Samba 설정 추가
  FILE *fp = fopen("/etc/samba/smb.conf", "a");
  if (fp) 
  {
    fprintf(fp, "\n[NAS_Storage_RAID%d]\n   path = /storage/share\n   writable = yes\n   force user = %s\n", level, username);
    fclose(fp);
  }

  // fstab 등록
  system("mdadm --detail --scan | tee -a /etc/mdadm/mdadm.conf");
  system("echo '/dev/md0  /storage  ext4  defaults,noatime  0  2' >> /etc/fstab");
}