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
 *
 * 이 프로그램은 대상 디스크의 데이터를 전부 지운다. 그래서 실제 파괴가 시작되기
 * 전에 대상 목록을 보여주고 확인을 받는다.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include "../lib/nas_lib.h"

#ifndef RAID_LEVEL
#define RAID_LEVEL 0
#endif

#define DISK_COUNT 4

/* 대상 디스크가 실제로 존재하는지 확인한다.
   없는 장치를 지우려 들면 mdadm 이 애매한 에러를 내므로 여기서 먼저 끊는다. */
static void require_disks(const char **disks, int count)
{
  for (int i = 0; i < count; i++)
  {
    if (access(disks[i], F_OK) != 0)
    {
      fprintf(stderr, "\n[오류] %s 장치를 찾을 수 없습니다.\n", disks[i]);
      fprintf(stderr, "lsblk 로 연결 상태를 확인하십시오.\n");
      exit(EXIT_FAILURE);
    }
  }
}

/* 파괴적 작업 직전 확인. YES 를 정확히 입력해야 진행한다.
   y/n 으로 받지 않는 이유는 습관적으로 엔터를 치는 것을 막기 위해서다. */
static void confirm_destruction(const char **disks, int count)
{
  printf("\n다음 디스크의 모든 데이터가 삭제됩니다.\n\n");
  for (int i = 0; i < count; i++)
    printf("  %s\n", disks[i]);

  printf("\n계속하려면 YES 를 입력하십시오: ");
  fflush(stdout);

  char answer[16];
  if (scanf("%15s", answer) != 1 || strcmp(answer, "YES") != 0)
  {
    printf("취소되었습니다. 디스크는 변경되지 않았습니다.\n");
    exit(EXIT_SUCCESS);
  }
}

/* 비밀번호를 화면에 표시하지 않고 읽는다.
   termios 로 터미널의 에코를 끄고, 읽은 뒤 원래 설정으로 되돌린다. */
static int read_password(char *buf, size_t size)
{
  struct termios old, new;
  if (tcgetattr(STDIN_FILENO, &old) != 0) return -1;

  new = old;
  new.c_lflag &= ~ECHO;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new) != 0) return -1;

  int ok = (fgets(buf, size, stdin) != NULL);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &old);
  printf("\n");

  if (!ok) return -1;
  buf[strcspn(buf, "\n")] = '\0';
  return 0;
}

int main(void) 
{
  const char *username = getenv("SUDO_USER");
  if (!username) username = "pi";

  printf("==== RAID %d 구성을 시작합니다 ====\n", RAID_LEVEL);

  if (geteuid() != 0) 
  {
    printf("sudo 권한이 필요합니다.\n");
    return 1;
  }

  const char *disks[] = {"/dev/sda", "/dev/sdb", "/dev/sdc", "/dev/sdd"};

  require_disks(disks, DISK_COUNT);
  confirm_destruction(disks, DISK_COUNT);

  char password[128];
  printf("Samba 비밀번호 입력: ");
  fflush(stdout);
  if (read_password(password, sizeof(password)) != 0 || password[0] == '\0')
  {
    fprintf(stderr, "\n[오류] 비밀번호를 읽지 못했습니다.\n");
    return 1;
  }

  // 초기화 및 생성.
  // mdadm --stop 은 정지할 배열이 없으면 실패하는 것이 정상이므로 확인하지 않는다.
  system("mdadm --stop /dev/md* 2>/dev/null");
  cleanup_disks(disks, DISK_COUNT);

  create_raid(RAID_LEVEL, disks, DISK_COUNT);

  // 포맷 및 마운트.
  // 여기부터는 앞 단계가 실패한 채로 다음이 돌면 안 된다.
  // 포맷이 실패했는데 마운트가 이어지면 원인이 훨씬 나중에 드러난다.
  check_exit(system("mkfs.ext4 -F /dev/md0"), "파일시스템 생성 실패");
  check_exit(system("mkdir -p /storage/share"), "마운트 디렉터리 생성 실패");
  check_exit(system("mount /dev/md0 /storage"), "RAID 마운트 실패");

  char chown_cmd[256];
  snprintf(chown_cmd, sizeof(chown_cmd), "chown -R %s:%s /storage/share", username, username);
  check_exit(system(chown_cmd), "공유 디렉터리 소유권 설정 실패");

  setup_samba_and_fstab(username, password, RAID_LEVEL);

  check_exit(system("systemctl restart smbd"), "Samba 재시작 실패");
  printf("\n[성공] ./raid%d 실행 완료!\n", RAID_LEVEL);

  return 0;
}