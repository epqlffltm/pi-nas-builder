#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../lib/utils.h"

// 함수 선언
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
    /* zfs 사용하지 않아서 필요 없어짐.
    printf("시스템의 총 메모리 크기를 확인합니다...\n");
    long mem_kb = get_total_memory_size_kb();
    
    if(mem_kb < 7500000) // 8GB 모델 체크
    {
        printf("시스템의 총 메모리 크기가 충분하지 않습니다. 최소 8GB 이상의 메모리가 필요합니다.\n");
        printf("현재 메모리 크기: %ld KB\n", mem_kb);
        exit(EXIT_FAILURE);
    }
    printf("시스템의 총 메모리 크기가 충분합니다: %ld KB\n", mem_kb);*/

    // 2. 패키지 업데이트 및 펌웨어
    printf("업데이트를 실행합니다...\n");
    check_exit(system("sudo apt update"), "패키지 업데이트 실패");
    
    printf("업그레이드를 실행합니다...\n");
    check_exit(system("sudo apt upgrade -y"), "패키지 업그레이드 실패");
    //check_exit(system("PIP_BREAK_SYSTEM_PACKAGES=1 apt upgrade -y"), "패키지 업그레이드 실패");//에러 문제로 주석처리함.

    
    printf("펌웨어 업데이트를 진행합니다...\n");
    check_exit(system("rpi-eeprom-update -a"), "펌웨어 업데이트 실패");
    
    printf("커널 헤더를 설치하겠습니다...\n");

    check_exit(system("apt install -y linux-headers-$(uname -r)"), "커널 헤더 설치 실패");

    // 3. 네트워크 부팅 지연 방지 (추가 추천!)
    // 네트워크가 연결될 때까지 부팅을 멈추는 서비스를 꺼서 '검은 화면' 대기를 방지합니다.
    printf("부팅 속도 최적화를 진행합니다...\n");
    system("systemctl disable systemd-networkd-wait-online.service");
    system("systemctl mask Raspberry-Pi-Custom-Network-State-Checker.service");

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

    // 5. mdadm 및 도구 설치
    printf("RAID 관리 도구(mdadm)를 설치합니다...\n");
    check_exit(system("sudo apt install mdadm -y"), "패키지 설치 실패");

    // 6. Samba 설치
    printf("Samba를 설치합니다...\n");
    check_exit(system("apt install -y samba samba-common-bin"), "Samba 설치 실패");

    //7. 방화벽 설정
    printf("방화벽을 설정합니다...\n");
    check_exit(system("apt install -y ufw"), "패키지 설치 실패");
    check_exit(system("ufw allow ssh"), "방화벽 ssh 설정 실패");
    check_exit(system("ufw allow 137,138/udp"), "방화벽 Samba UDP 설정 실패");
    check_exit(system("ufw allow 139,445/tcp"), "방화벽 Samba TCP 설정 실패");

    printf("방화벽을 활성화합니다...\n");
    check_exit(system("ufw --force enable"), "방화벽 활성화 실패");

    /*패키지 꼬임 문제 발생. 쓰지 말 것!
    //8. radxa Penta SATA Hat 드라이버 설치
    printf("Radxa Penta SATA Hat 드라이버를 설치합니다...\n");
    check_exit(system("apt install -y wget"), "wget 설치 실패");
    check_exit(system("wget -N https://github.com/radxa/rockpi-penta/releases/download/v0.2.2/rockpi-penta-0.2.2.deb"), "드라이버 다운로드 실패");
    check_exit(system("PIP_BREAK_SYSTEM_PACKAGES=1 apt install -y ./rockpi-penta-0.2.2.deb"), "드라이버 설치 실패");
    check_exit(system("rm rockpi-penta-0.2.2.deb"), "드라이버 설치 파일 삭제 실패");
    */

    // 9. 한글화
    printf("시스템 한글화를 진행합니다...\n");
    check_exit(system("apt install ibus ibus-hangul -y"), "한글 입력기 설치 실패");
    check_exit(system("apt install -y fonts-nanum fonts-unfonts-core"), "한글 폰트 설치 실패");
    check_exit(system("sed -i 's/# ko_KR.UTF-8 UTF-8/ko_KR.UTF-8 UTF-8/' /etc/locale.gen && locale-gen"), "로케일 생성 실패");
    printf("시간대를 Asia/Seoul로 설정합니다...\n");
    check_exit(system("timedatectl set-timezone Asia/Seoul"), "시간대 설정 실패");

    // 10. 완료 및 재부팅
    printf("설치가 성공적으로 완료되었습니다! 시스템을 재부팅하겠습니다...\n");
    sync();
    sleep(2); // 메시지를 읽을 시간 확보
    check_exit(system("reboot"), "시스템 재부팅 실패");

    return 0;
}