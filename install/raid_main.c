// install/raid_main.c

/*
 * NAS 구축 2단계 — RAID 구성.
 *
 * 디스크 초기화부터 RAID 생성, 포맷, 마운트, Samba 공유 개방까지를 담당한다.
 *
 * RAID 레벨은 실행 인자가 아니라 컴파일 타임 매크로(RAID_LEVEL)로 받는다.
 * 한 번 정하면 바뀌지 않는 값이라 실행할 때마다 판단할 이유가 없고, 이렇게 하면
 * 각 바이너리가 자기 레벨의 코드만 갖게 된다. 실행 파일 이름(raid0/1/5/10)이
 * 곧 무엇을 만드는지를 말하므로 잘못된 인자로 의도하지 않은 RAID 를 구성할
 * 여지도 없다. 빌드 타겟은 Makefile 참고.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../lib/nas_lib.h"

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