#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include "../lib/utils.h"

int main(void) 
{
  printf("NAS 저장소 구성을 시작합니다...\n");
    // 1. 권한 확인
    printf("루트 권한을 확인합니다...\n");
    if (geteuid() != 0) 
    {
        printf("이 프로그램은 sudo 권한으로 실행해야 합니다.\n");
        return 1;
    }

    // 2. 실제 사용자 이름 가져오기
    printf("실제 사용자 이름을 확인합니다...\n");
    const char *username = getenv("SUDO_USER");
    if (username == NULL) 
    {
        struct passwd *pw = getpwuid(getuid());
        username = (pw) ? pw->pw_name : "pi";
    }

    // 3. Samba 비밀번호 입력
    printf("Samba 사용자 비밀번호를 설정합니다...\n");
    char password[128];
    printf("사용자 [%s]의 Samba 접속 비밀번호를 설정하세요: ", username);
    if (scanf("%127s", password) != 1) 
    {
        printf("입력 오류 발생\n");
        return 1;
    }

    printf("\n[%s] 계정으로 NAS 저장소 구성을 시작합니다...\n", username);

    // 4. 기존 RAID 및 디스크 점유 해제
    printf("기존 RAID 배열 확인 및 디스크 점유를 해제합니다...\n");
    // 활성화된 모든 md 장치 중지
    system("mdadm --stop /dev/md* 2>/dev/null"); 

    // 각 디스크 초기화
    printf("디스크 초기화 중...\n");
    const char *disks[] = {"/dev/sda", "/dev/sdb", "/dev/sdc", "/dev/sdd"};
    for (int i = 0; i < 4; i++) 
    {
        char cmd[256];
        printf("[%s] 초기화 중...\n", disks[i]);
        
        // RAID 메타데이터(Superblock) 강제 제거
        printf("[%s] RAID 메타데이터 제거 중...\n", disks[i]);
        sprintf(cmd, "mdadm --zero-superblock %s 2>/dev/null", disks[i]);
        system(cmd);
        
        // 파일시스템 지문 및 파티션 테이블 제거
        printf("[%s] 파일시스템 및 파티션 테이블 제거 중...\n", disks[i]);
        sprintf(cmd, "wipefs -a %s && sgdisk --zap-all %s", disks[i], disks[i]);
        system(cmd);
    }

    // 5. mdadm RAID 0 생성
    printf("RAID 0 배열(/dev/md0)을 생성합니다...\n");
    check_exit(system("mdadm --create --verbose /dev/md0 --level=0 --raid-devices=4 /dev/sda /dev/sdb /dev/sdc /dev/sdd --run"), "RAID 생성 실패");

    // 6. 파일시스템 생성 및 마운트
    printf("ext4 파일시스템 생성 중 (시간이 다소 소요될 수 있습니다)...\n");
    check_exit(system("mkfs.ext4 -F /dev/md0"), "포맷 실패");
    
    system("mkdir -p /storage");
    check_exit(system("mount /dev/md0 /storage"), "마운트 실패");

    // 7. 공유 폴더 생성 및 권한 설정
    printf("공유 폴더를 생성하고 권한을 설정합니다...\n");
    system("mkdir -p /storage/share");
    char chown_cmd[256];
    sprintf(chown_cmd, "chown -R %s:%s /storage/share", username, username);
    system(chown_cmd);

    // 8. Samba 사용자 등록 및 설정 (버퍼 크기 512로 확장)
    printf("Samba 계정 등록 중...\n");
    char smb_user_cmd[512]; 
    sprintf(smb_user_cmd, "(echo \"%s\"; echo \"%s\") | smbpasswd -s -a %s", password, password, username);
    system(smb_user_cmd);

    printf("Samba 설정 파일을 업데이트합니다...\n");
    FILE *fp = fopen("/etc/samba/smb.conf", "a");
    if (fp != NULL) {
        fprintf(fp, "\n[NAS_Storage]\n");
        fprintf(fp, "   path = /storage/share\n");
        fprintf(fp, "   browsable = yes\n");
        fprintf(fp, "   writable = yes\n");
        fprintf(fp, "   guest ok = no\n");
        fprintf(fp, "   force user = %s\n", username);
        fclose(fp);
    }

    // 9. 자동 마운트 설정 (RAID 정보 저장)
    printf("시스템 부팅 설정을 업데이트합니다...\n");
    system("mdadm --detail --scan | tee -a /etc/mdadm/mdadm.conf");
    system("echo '/dev/md0  /storage  ext4  defaults,noatime  0  2' >> /etc/fstab");

    // 10. 바탕화면 바로가기 생성 (Desktop 전용)
    printf("Creating shortcut on Desktop...\n");
    char desktop_link[512];
    sprintf(desktop_link, "ln -sf /storage/share /home/%s/Desktop/NAS_Storage", username);
    system(desktop_link);

    // 바로가기 파일 소유권 변경
    char chown_link[512];
    sprintf(chown_link, "chown -h %s:%s /home/%s/Desktop/NAS_Storage", username, username, username);
    system(chown_link);

    // 10. 서비스 재시작
    check_exit(system("systemctl restart smbd"), "Samba 재시작 실패");

    printf("\n[성공] 모든 작업이 완료되었습니다!\n");
    printf("아이디: %s\n", username);
    printf("경로: /storage/share\n");

    return 0;
}