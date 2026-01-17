#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include "../lib/utils.h"

int main(void) 
{
    // 1. 권한 확인
    if (geteuid() != 0) 
    {
        printf("이 프로그램은 sudo 권한으로 실행해야 합니다.\n");
        return 1;
    }

    // 2. 실제 사용자 이름 자동으로 가져오기
    // sudo ./install_app을 실행한 원래 사용자의 ID를 가져옵니다.
    const char *username = getenv("SUDO_USER");
    
    // 만약 root에서 직접 실행했거나 SUDO_USER가 없다면 현재 실행자 확인
    if (username == NULL) 
    {
        struct passwd *pw = getpwuid(getuid());
        username = (pw) ? pw->pw_name : "pi";
    }

    // 3. Samba 비밀번호만 입력 받기
    char password[128];
    printf("사용자 [%s]의 Samba 접속 비밀번호를 설정하세요: ", username);
    if (scanf("%127s", password) != 1) 
    {
        printf("입력 오류가 발생했습니다.\n");
        return 1;
    }

    printf("\n[%s] 계정으로 NAS 저장소 구성을 시작합니다...\n", username);

    // 4. 디스크 초기화 (기존 데이터 파괴)
    check_exit(system("lsblk"), "디스크 목록 조회 실패");
    const char *disks[] = {"/dev/sda", "/dev/sdb", "/dev/sdc", "/dev/sdd"};
    for (int i = 0; i < 4; i++) 
    {
        char cmd[128];
        sprintf(cmd, "wipefs -a %s && sgdisk --zap-all %s", disks[i], disks[i]);
        system(cmd);
    }

    // 5. mdadm RAID 0 생성
    printf("RAID 0 배열 생성 중...\n");
    check_exit(system("mdadm --create --verbose /dev/md0 --level=0 --raid-devices=4 /dev/sda /dev/sdb /dev/sdc /dev/sdd --run"), "RAID 생성 실패");

    // 6. 파일시스템 생성 및 마운트
    check_exit(system("mkfs.ext4 -F /dev/md0"), "포맷 실패");
    system("mkdir -p /storage");
    check_exit(system("mount /dev/md0 /storage"), "마운트 실패");

    // 7. 공유 폴더 생성 및 권한 부여
    // sudo를 쓴 사용자가 주인(Owner)이 되도록 설정합니다.
    system("mkdir -p /storage/share");
    char chown_cmd[128];
    sprintf(chown_cmd, "chown -R %s:%s /storage/share", username, username);
    system(chown_cmd);

    // 8. Samba 사용자 등록 및 설정
    char smb_user_cmd[512];
    sprintf(smb_user_cmd, "(echo \"%s\"; echo \"%s\") | smbpasswd -s -a %s", password, password, username);
    system(smb_user_cmd);

    FILE *fp = fopen("/etc/samba/smb.conf", "a");
    if (fp != NULL) 
    {
        fprintf(fp, "\n[NAS_Storage]\n");
        fprintf(fp, "   path = /storage/share\n");
        fprintf(fp, "   browsable = yes\n");
        fprintf(fp, "   writable = yes\n");
        fprintf(fp, "   guest ok = no\n");
        fprintf(fp, "   force user = %s\n", username); // 자동으로 가져온 이름 적용
        fclose(fp);
    }

    // 9. 자동 마운트 설정 및 서비스 재시작
    system("mdadm --detail --scan | tee -a /etc/mdadm/mdadm.conf");
    system("echo '/dev/md0  /storage  ext4  defaults,noatime  0  2' >> /etc/fstab");
    check_exit(system("systemctl restart smbd"), "Samba 재시작 실패");

    printf("\n설정이 완료되었습니다!\n");
    printf("접속 아이디: %s\n", username);
    printf("경로: \\\\라즈베리IP\\NAS_Storage\n");

    return 0;
}