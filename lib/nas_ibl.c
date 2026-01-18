#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../lib/nas_ibl.h"

void check_exit(int status, const char *message) 
{
  if (status != 0) 
  {
    fprintf(stderr, "\n[오류] %s (코드: %d)\n", message, status);
    exit(EXIT_FAILURE);
  }
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