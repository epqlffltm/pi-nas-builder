#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../lib/nas_ibl.h"

int main(void) 
{
  // 컴파일 시 RAID_LEVEL이 정의되지 않았을 경우를 대비
  #ifndef RAID_LEVEL
  #define RAID_LEVEL 0 
  #endif

  const char *username = getenv("SUDO_USER");
  if (!username) username = "pi";

  printf("==== RAID %d 구성을 시작합니다 ====\n", RAID_LEVEL);

  if (geteuid() != 0) 
  {
    printf("sudo 권한이 필요합니다.\n");
    return 1;
  }

  char password[128];
  printf("Samba 비밀번호 입력: ");
  scanf("%127s", password);

  // 초기화 및 생성
  system("mdadm --stop /dev/md* 2>/dev/null");
  const char *disks[] = {"/dev/sda", "/dev/sdb", "/dev/sdc", "/dev/sdd"};
  cleanup_disks(disks, 4);

  create_raid(RAID_LEVEL, disks, 4);

  // 포맷 및 마운트
  system("mkfs.ext4 -F /dev/md0");
  system("mkdir -p /storage/share");
  system("mount /dev/md0 /storage");
  
  char chown_cmd[256];
  sprintf(chown_cmd, "chown -R %s:%s /storage/share", username, username);
  system(chown_cmd);
  
  setup_samba_and_fstab(username, password, RAID_LEVEL);
  
  system("systemctl restart smbd");
  printf("\n[성공] ./raid%d 실행 완료!\n", RAID_LEVEL);

  return 0;
}