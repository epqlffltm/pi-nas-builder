// lib/nas_lib.c

/*
 * 공통 유틸리티와 NAS 구성 핵심 로직의 구현부.
 *
 * 이 파일의 함수들은 대부분 system()으로 외부 명령을 부른다. C로 감싼 이유는
 * 각 단계의 실패를 종료 코드로 받아 다음 단계로 넘어가지 않게 막기 위해서다.
 * mdadm --create 가 실패했는데 mkfs.ext4 가 이어서 도는 것이 가장 위험하다.
 *
 * 반환값을 확인하지 않는 호출이 몇 군데 있는데, 전부 "실패가 정상인" 경우다.
 * 해당 위치에 이유를 적어 두었다.
 *
 * 설정 파일에 무언가를 덧붙이는 코드는 모두 grep_config 으로 먼저 확인한다.
 * 이 도구는 RAID 레벨을 바꾸거나 설정을 손볼 때 다시 실행되기 때문에,
 * 같은 줄이 쌓이면 부팅이나 공유가 깨진다.
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
   같은 줄을 중복으로 쓰지 않기 위해 쓴다.
   파일이 없으면 "없다"로 본다 — 호출부가 곧 새로 만들기 때문이다.
   주석 처리된 줄은 실제로 적용되지 않으므로 건너뛴다.
   부분 문자열로 찾으므로 중복은 막지만 기존 줄의 내용이 올바른지는 검증하지 않는다. */
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

/* 디스크에 남은 이전 흔적을 지운다.
   반환값을 확인하지 않는 이유: 지울 대상이 없는 깨끗한 디스크에서도 실행되며,
   그 경우 zero-superblock 은 실패를 반환하는 것이 정상이다. 여기서 중단하면
   새 디스크로는 RAID 를 만들 수 없다. 실제 실패는 다음 단계인 create_raid 의
   mdadm --create 에서 드러난다. */
void cleanup_disks(const char **disks, int count) 
{
  for (int i = 0; i < count; i++) 
  {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "mdadm --zero-superblock %s 2>/dev/null", disks[i]);
    system(cmd);
    snprintf(cmd, sizeof(cmd), "wipefs -a %s && sgdisk --zap-all %s", disks[i], disks[i]);
    system(cmd);
  }
}

void create_raid(int level, const char **disks, int count) 
{
  char disk_list[512] = "";
  for (int i = 0; i < count; i++)
  {
    // 경계를 넘기면 조용히 잘린 장치 목록으로 RAID 를 만들게 되므로 여기서 끊는다.
    if (strlen(disk_list) + strlen(disks[i]) + 2 > sizeof(disk_list))
      check_exit(1, "디스크 목록이 너무 깁니다");

    strcat(disk_list, disks[i]);
    strcat(disk_list, " ");
  }

  char cmd[1024];
  snprintf(cmd, sizeof(cmd), "mdadm --create --verbose /dev/md0 --level=%d --raid-devices=%d %s --run", level, count, disk_list);
  check_exit(system(cmd), "RAID 생성 실패");
}

void setup_samba_and_fstab(const char *username, const char *password, int level) 
{
  /* Samba 계정 등록.
     비밀번호를 명령 문자열에 넣지 않고 popen 의 표준입력으로 넘긴다.
     문자열에 넣으면 따옴표나 $ 같은 셸 특수문자가 명령의 의미를 바꿀 수 있고,
     실행 중 ps 로 비밀번호가 그대로 보인다. */
  char smb_cmd[256];
  snprintf(smb_cmd, sizeof(smb_cmd), "smbpasswd -s -a %s", username);

  FILE *pw = popen(smb_cmd, "w");
  if (pw == NULL)
  {
    check_exit(1, "smbpasswd 실행 실패");
  }
  fprintf(pw, "%s\n%s\n", password, password);
  check_exit(pclose(pw), "Samba 계정 등록 실패");

  // Samba 설정 추가.
  // 같은 이름의 섹션이 두 번 들어가면 Samba 가 뒤엣것만 읽어 설정이 조용히 무시된다.
  char section[64];
  snprintf(section, sizeof(section), "[NAS_Storage_RAID%d]", level);

  if (!grep_config("/etc/samba/smb.conf", section))
  {
    FILE *fp = fopen("/etc/samba/smb.conf", "a");
    if (fp == NULL)
      check_exit(1, "smb.conf 를 열 수 없습니다");

    fprintf(fp, "\n%s\n   path = /storage/share\n   writable = yes\n   force user = %s\n", section, username);
    fclose(fp);
  }

  // mdadm.conf 등록. 같은 배열 정의가 중복되면 부팅 시 배열 조립에서 경고가 난다.
  if (!grep_config("/etc/mdadm/mdadm.conf", "/dev/md0"))
    check_exit(system("mdadm --detail --scan | tee -a /etc/mdadm/mdadm.conf"), "mdadm.conf 등록 실패");

  // fstab 등록. 같은 마운트가 두 줄이면 부팅이 실패할 수 있다.
  if (!grep_config("/etc/fstab", "/dev/md0"))
    check_exit(system("echo '/dev/md0  /storage  ext4  defaults,noatime  0  2' >> /etc/fstab"), "fstab 등록 실패");
}