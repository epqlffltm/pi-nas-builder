#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include "../lib/utils.h"

int main(void) 
{
    // 1. 권한 및 사용자 정보 확인
    if (geteuid() != 0) 
    {
        printf("이 프로그램은 sudo 권한으로 실행해야 합니다.\n");
        return 1;
    }

    // 실제 사용자 이름 가져오기 (Samba force user 용)
    const char *username;
    struct passwd *pw = getpwuid(getuid());
    if (pw) username = pw->pw_name;
    else username = "pi"; // 기본값

    printf("Penta SATA Hat SSD 저장소 구성을 시작합니다...\n");

    // 2. 디스크 초기화 (기존 데이터 파괴)
    printf("디스크 데이터를 초기화합니다...\n");
    const char *disks[] = {"/dev/sda", "/dev/sdb", "/dev/sdc", "/dev/sdd"};
    for (int i = 0; i < 4; i++) 
    {
        char cmd[128];
        sprintf(cmd, "wipefs -a %s", disks[i]);
        system(cmd);
        sprintf(cmd, "sgdisk --zap-all %s", disks[i]);
        system(cmd);
    }

    // 3. ZFS 풀 생성 (Stripe)
    printf("ZFS 풀(storage)을 생성합니다...\n");
    printf("stripe 모드로 설정됩니다. (데이터 보호 없음)\n");
    check_exit(system("zpool create -f storage /dev/sda /dev/sdb /dev/sdc /dev/sdd"), "ZFS 풀 생성 실패");

    // 4. 권한 설정 및 심볼릭 링크
    printf("마운트 경로 권한 설정 및 링크 생성 중...\n");
    char chown_cmd[128], ln_cmd[128];
    sprintf(chown_cmd, "chown -R %s:%s /storage/share", username, username);
    system(chown_cmd);
    
    // 홈 디렉토리에 링크 생성
    sprintf(ln_cmd, "ln -sf /storage/share /home/%s/share", username);
    system(ln_cmd);

    // 5. Samba 설정 추가
    printf("Samba 공유 설정을 추가합니다...\n");
    FILE *fp = fopen("/etc/samba/smb.conf", "a");
    if (fp == NULL) 
    {
        perror("Samba 설정 파일을 열 수 없습니다");
        return 1;
    }

    fprintf(fp, "\n[NAS_Storage]\n");
    fprintf(fp, "   path = /storage\n");
    fprintf(fp, "   browsable = yes\n");
    fprintf(fp, "   writable = yes\n");
    fprintf(fp, "   guest ok = yes\n");
    fprintf(fp, "   force user = %s\n", username);
    fclose(fp);

    // 6. 서비스 재시작
    printf("Samba 서비스를 재시작합니다...\n");
    check_exit(system("systemctl restart smbd"), "Samba 재시작 실패");

    printf("\n모든 작업이 완료되었습니다!\n");
    printf("마운트 위치: /storage\n");
    printf("Samba 이름: NAS_Storage (게스트 접속 가능)\n");

    return 0;
}