#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 함수 선언
void check_exit(int status, const char *message);
int grep_config(const char *file_name, const char *search);
long get_total_memory_size_kb(void);

int main(void)
{
    // 1. 권한 확인
    if(geteuid() != 0)
    {
        printf("이 프로그램은 root 권한으로 실행되어야 합니다.\n");
        printf("sudo를 사용하여 다시 실행하십시오.\n");
        exit(EXIT_FAILURE);
    }

    // 2. 메모리 크기 확인
    printf("시스템의 총 메모리 크기를 확인합니다...\n");
    long mem_kb = get_total_memory_size_kb();
    
    if(mem_kb < 7500000) // 8GB 모델 체크
    {
        printf("시스템의 총 메모리 크기가 충분하지 않습니다. 최소 8GB 이상의 메모리가 필요합니다.\n");
        printf("현재 메모리 크기: %ld KB\n", mem_kb);
        exit(EXIT_FAILURE);
    }
    printf("시스템의 총 메모리 크기가 충분합니다: %ld KB\n", mem_kb);

    // 3. 패키지 업데이트 및 펌웨어
    printf("업데이트를 실행합니다...\n");
    check_exit(system("apt update"), "패키지 업데이트 실패");
    
    printf("업그레이드를 실행합니다...\n");
    check_exit(system("apt upgrade -y"), "패키지 업그레이드 실패");
    
    printf("펌웨어 업데이트를 진행합니다...\n");
    check_exit(system("rpi-eeprom-update -a"), "펌웨어 업데이트 실패");
    
    printf("커널 헤더를 설치하겠습니다...\n");

    check_exit(system("apt install -y linux-headers-$(uname -r)"), "커널 헤더 설치 실패");

    // 4. PCIe 설정
    printf("PCIe 설정을 확인합니다...\n");
    const char *CONFIG_FILE = "/boot/firmware/config.txt";
    
    FILE *file = fopen(CONFIG_FILE, "a");
    if(file == NULL)
    {
        fprintf(stderr, "오류: 파일 %s을(를) 열 수 없습니다.\n", CONFIG_FILE);
        exit(EXIT_FAILURE);
    }

    if(!grep_config(CONFIG_FILE, "dtparam=pciex1"))
    {
        printf("PCIe 설정 추가 중...\n");
        fprintf(file, "\ndtparam=pciex1\n");
    }

    if(!grep_config(CONFIG_FILE, "dtparam=pciex1_gen=3"))
    {
        printf("PCIe Gen3 설정 추가 중...\n");
        fprintf(file, "dtparam=pciex1_gen=3\n");
    }
    fclose(file);

    // 5. ZFS 설치
    printf("ZFS를 설치합니다...\n");
    printf("시간이 오래 소요됩니다...\n");
    check_exit(system("apt install -y zfs-dkms zfsutils-linux"), "패키지 설치 실패");

    //6. ZFS 모듈 로드
    printf("ZFS 모듈을 로드합니다...\n");
    check_exit(system("modprobe zfs"), "ZFS 모듈 로드 실패");

    // 7. Samba 설치
    printf("Samba를 설치합니다...\n");
    check_exit(system("apt install -y samba samba-common-bin"), "Samba 설치 실패");

    //8. 방화벽 설정
    printf("방화벽을 설정합니다...\n");
    check_exit(system("ufw allow ssh"), "방화벽 ssh 설정 실패");
    check_exit(system("ufw allow 137,138/udp"), "방화벽 Samba UDP 설정 실패");
    check_exit(system("ufw allow 139,445/tcp"), "방화벽 Samba TCP 설정 실패");

    printf("방화벽을 활성화합니다...\n");
    check_exit(system("ufw --force enable"), "방화벽 활성화 실패");

    //9. radxa Penta SATA Hat 드라이버 설치
    printf("Radxa Penta SATA Hat 드라이버를 설치합니다...\n");
    check_exit(system("apt install -y wget"), "wget 설치 실패");
    check_exit(system("wget -N https://github.com/radxa/rockpi-penta/releases/download/v0.2.2/rockpi-penta-0.2.2.deb"), "드라이버 다운로드 실패");
    check_exit(system("apt install -i rockpi-penta-0.2.2.deb"), "드라이버 설치 실패");
    check_exit(system("rm rockpi-penta-0.2.2.deb"), "드라이버 설치 파일 삭제 실패");

    // 10. 완료 및 재부팅
    printf("설치가 성공적으로 완료되었습니다! 시스템을 재부팅하겠습니다...\n");
    sync();
    sleep(2); // 메시지를 읽을 시간 확보
    check_exit(system("reboot"), "시스템 재부팅 실패");

    return 0;
}

// 명령어 실행 성공 여부 확인
void check_exit(int status, const char *message)
{
    if (status != 0)
    {
        fprintf(stderr, "\n오류 발생: %s (코드: %d)\n", message, status);
        exit(EXIT_FAILURE);
    }
}
// 설정 파일에서 특정 문자열 검색
int grep_config(const char *file_name, const char *search)
{
    FILE *file = fopen(file_name, "r");
    if (file == NULL) return 0;

    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), file))
    {
        if(strstr(line, search) != NULL)
        {
            found = 1;
            break;
        }
    }
    fclose(file);
    return found;
}

// 메모리 정보 가져오기 구현
long get_total_memory_size_kb(void)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) return -1;

    char label[64];
    long mem_kb = 0;
    if (fscanf(fp, "%s %ld", label, &mem_kb) != 2)
    {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return mem_kb;
}