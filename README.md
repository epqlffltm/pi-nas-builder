# Raspberry Pi NAS Builder - Automated RAID Configuration Tool
## One-Command Setup for Radxa Penta SATA HAT

![Radxa Penta SATA HAT](img/hat.webp)

[![C](https://img.shields.io/badge/C-00599C?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Raspberry%20Pi%205-C51A4A)](https://www.raspberrypi.com/)

> **자동화된 NAS 구축 도구**  
> 복잡한 리눅스 명령어와 RAID 설정을 단 한 줄의 명령어로 완성

---

## 📊 프로젝트 개요

Radxa 공식 문서의 명령어들을 **C 프로그램으로 자동화**하고, 방어적 프로그래밍으로 안정성을 강화한 NAS 구축 도구입니다.

### 해결한 문제
```bash
# 기존 방법: Radxa 공식 문서 보면서 30개 명령어 수동 실행
sudo apt update
sudo apt install mdadm samba -y
sudo mdadm --create /dev/md0 --level=5 ...
sudo mkfs.ext4 /dev/md0
sudo mkdir /storage
sudo mount /dev/md0 /storage
sudo vim /etc/samba/smb.conf
# ... (Radxa 문서에서 복사 → 붙여넣기 반복)

# 이 프로젝트: 단 두 줄로 자동화
sudo ./setup_app
sudo ./raid5
```

### 핵심 성과
| 항목 | 수치 | 비고 |
|-----|------|------|
| **설정 시간 단축** | 1시간 → 5분 | 12배 빠름 |
| **개발 시간** | 1일 | 빠른 학습 + 구현 |
| **자동화 단계** | 30+ 명령어 | Samba, RAID, 방화벽 등 |
| **지원 RAID** | 0, 1, 5, 10 | 사용자 선택 가능 |
| **C 시스템 프로그래밍** | system(), sprintf(), fopen() | 처음 배워서 적용 |

---

## 🎯 기술적 도전과 해결

### 가장 큰 도전: C로 명령어 자동 실행 + 방어적 프로그래밍

**문제:** Radxa 공식 문서의 명령어를 그대로 따라하면 되지만, C로 자동화하는 게 생소했음

```bash
# Radxa 공식 문서 (잘 되어있음)
sudo mdadm --create /dev/md0 --level=5 ...
sudo mkfs.ext4 -F /dev/md0
sudo mount /dev/md0 /storage

# → 이걸 수동으로 치는 건 쉬움
# → C로 자동화하는 건? 처음!
```

---

### 배워야 했던 것들

#### 1. C의 system() 호출 (생소했음)
```c
// Bash 스크립트 (익숙함)
#!/bin/bash
sudo apt install mdadm
sudo mdadm --create /dev/md0 ...

// C로 변환 (배워야 했음)
#include <stdlib.h>
system("apt install mdadm");
system("mdadm --create /dev/md0 ...");
```

**낯선 점:**
- `system()` 함수 자체가 처음
- 리턴값 체크는 어떻게?
- 에러 나면 어떻게 처리?

---

#### 2. 방어적 프로그래밍 (핵심!)
```c
// 나쁜 코드 (에러 무시)
system("mdadm --create /dev/md0 --level=5 ...");
system("mkfs.ext4 /dev/md0");
// 첫 줄이 실패해도 두 번째 줄 실행됨!

// 좋은 코드 (방어적)
if (system("mdadm --create /dev/md0 --level=5 ...") != 0) {
    fprintf(stderr, "RAID 생성 실패!\n");
    exit(1);  // 여기서 멈춤
}

// 더 나은 코드 (헬퍼 함수)
void check_exit(int ret, const char *msg) {
    if (ret != 0) {
        fprintf(stderr, "오류: %s\n", msg);
        exit(EXIT_FAILURE);
    }
}

check_exit(system("mdadm --create ..."), "RAID 생성 실패");
check_exit(system("mkfs.ext4 /dev/md0"), "포맷 실패");
```

---

#### 3. 입력 검증 (버퍼 오버플로우 방지)
```c
// 나쁜 코드
char password[128];
scanf("%s", password);  // 위험! 128자 이상 입력 가능

// 좋은 코드 (방어적)
char password[128];
scanf("%127s", password);  // 127자만 받음 (NULL 종료 고려)

// 명령어에 넣을 때도 검증
if (strlen(password) == 0) {
    fprintf(stderr, "비밀번호를 입력하세요\n");
    exit(1);
}
```

---

#### 4. 파일 존재 검증
```c
// 나쁜 코드
FILE *fp = fopen("/etc/samba/smb.conf", "a");
fprintf(fp, "[NAS_Storage]\n");  // fp가 NULL이면 크래시!

// 좋은 코드 (방어적)
FILE *fp = fopen("/etc/samba/smb.conf", "a");
if (fp == NULL) {
    perror("파일 열기 실패");
    exit(1);
}
fprintf(fp, "[NAS_Storage]\n");
fclose(fp);
```

---

#### 5. 디스크 존재 확인
```c
// RAID 생성 전 디스크 체크
const char *disks[] = {"/dev/sda", "/dev/sdb", "/dev/sdc", "/dev/sdd"};

for (int i = 0; i < 4; i++) {
    if (access(disks[i], F_OK) != 0) {
        fprintf(stderr, "%s 디스크가 없습니다!\n", disks[i]);
        exit(1);
    }
}

// 이제 안전하게 RAID 생성
system("mdadm --create /dev/md0 ...");
```

---

### 학습 과정 (1일)

```
오전 (4시간):
1. Radxa 공식 문서 읽기 (1시간)
   - 어떤 명령어들이 필요한지 정리
   
2. C system() 함수 검색 (1시간)
   - 사용법, 리턴값, 에러 처리
   
3. 방어적 프로그래밍 패턴 학습 (2시간)
   - NULL 체크, 버퍼 오버플로우 방지
   - check_exit 헬퍼 함수 설계

오후 (4시간):
4. 코드 작성 (2시간)
   - setup.c, raid_main.c 구현
   
5. 실제 하드웨어 테스트 (2시간)
   - Raspberry Pi 5에서 실행
   - 버그 수정 (경로 오류, 권한 문제 등)
```

**총 8시간 (1일)**

---

### 쉬웠던 부분: 명령어 로직

**이유:** Radxa 공식 문서가 잘 되어있음!

```bash
# Radxa 문서에 모든 명령어가 다 있음
https://docs.radxa.com/en/accessories/storage/penta-sata-hat

# 예시:
sudo mdadm --create /dev/md0 --level=5 --raid-devices=4 /dev/sda /dev/sdb /dev/sdc /dev/sdd
sudo mkfs.ext4 -F /dev/md0
sudo mkdir -p /storage
sudo mount /dev/md0 /storage

# → 이걸 C로 옮기기만 하면 됨!
```

**즉:**
- RAID 명령어: Radxa 문서 복사
- Samba 설정: 인터넷 검색하면 나옴
- 방화벽 설정: ufw 명령어는 간단

→ **로직은 쉬움, C 문법과 방어적 프로그래밍이 핵심!**

---

### 성과

| 항목 | 결과 |
|-----|------|
| **C system() 호출** | ✅ 완전 이해 |
| **방어적 프로그래밍** | ✅ 에러 처리, NULL 체크, 버퍼 검증 |
| **입력 검증** | ✅ 버퍼 오버플로우 방지 |
| **파일 I/O** | ✅ NULL 체크, fclose 관리 |
| **자동화 완성도** | ✅ 30+ 명령어 자동화 |
| **개발 시간** | ✅ 1일 (8시간) |

**핵심 교훈:**
> "좋은 문서(Radxa) + C 자동화 + 방어적 프로그래밍 = 안정적인 도구"

---

## 🏗️ 시스템 아키텍처

```
사용자 입력 (sudo ./raid5)
    ↓
┌─────────────────────────────────────────┐
│         setup.c (초기 환경 설정)          │
│  ├─ 시스템 업데이트                       │
│  ├─ mdadm, Samba 설치                   │
│  ├─ 방화벽 설정 (UFW)                    │
│  ├─ PCIe Gen3 활성화                     │
│  └─ 한글화 및 시간대 설정                 │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│      raid_main.c (RAID 구성 엔진)        │
│  ├─ 디스크 초기화 (wipefs, sgdisk)       │
│  ├─ RAID 배열 생성 (mdadm)              │
│  ├─ 파일시스템 포맷 (mkfs.ext4)          │
│  ├─ 자동 마운트 설정 (fstab)             │
│  └─ Samba 공유 설정                      │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│         /storage/share (NAS)            │
│  - Windows/Mac에서 네트워크 드라이브로    │
│  - 자동 백업, 미디어 서버 등 활용 가능     │
└─────────────────────────────────────────┘
```

---

## 🚀 빠른 시작

### 필수 요구사항
```
Hardware:
- Raspberry Pi 5 (4GB+ 권장)
- Radxa Penta SATA HAT
- SATA SSD 2.5" x 4개 (HDD도 가능)
- 12V 5A 전원 어댑터 (5525 잭)

Software:
- Raspberry Pi OS Lite (64-bit)
- GCC 컴파일러
```

### 설치 및 실행
```bash
# 1. 저장소 복제
git clone https://github.com/epqlffltm/pi-nas-builder.git
cd pi-nas-builder

# 2. 컴파일
make

# 3. 초기 환경 설정 (최초 1회만)
sudo ./setup_app
# 시스템 재부팅됨

# 4. RAID 구성 (원하는 레벨 선택)
sudo ./raid5    # RAID 5 (권장)
# 또는
sudo ./raid0    # RAID 0 (성능 최우선)
sudo ./raid1    # RAID 1 (안정성 최우선)
sudo ./raid10   # RAID 10 (성능 + 안정성)

# 5. Windows에서 접속
# \\<라즈베리파이_IP>\NAS_Storage
```

---

## 📂 프로젝트 구조

```
pi-nas-builder/
├── Makefile              # 빌드 자동화 스크립트
├── README.md             # 프로젝트 문서
│
├── setup/                # 🔧 초기 환경 설정
│   └── setup.c           # 시스템 업데이트, 패키지 설치
│
├── install/              # 💾 RAID 구성 엔진
│   ├── install.c         # RAID 0 전용 (레거시)
│   └── raid_main.c       # RAID 0/1/5/10 통합
│
└── lib/                  # 📚 공통 라이브러리
    ├── nas_lib.c         # RAID 생성, Samba 설정 함수
    ├── nas_lib.h         # 헤더 파일
    └── utils.h           # 유틸리티 함수
```

---

## 💾 RAID 레벨 비교

| RAID | 최소 디스크 | 가용 용량 | 성능 | 안정성 | 추천 용도 |
|------|-----------|----------|------|--------|---------|
| **RAID 0** | 2개 | 100% (4TB) | ⭐⭐⭐⭐⭐ | ❌ 없음 | 임시 작업, 캐시 |
| **RAID 1** | 2개 | 50% (2TB) | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 중요 백업 |
| **RAID 5** | 3개 | 75% (3TB) | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | **일반 NAS 권장** |
| **RAID 10** | 4개 | 50% (2TB) | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 고성능 서버 |

**예시:** 1TB SSD 4개 사용 시

---

## 🔧 기술적 구현 상세

### 디스크 초기화 과정
```c
const char *disks[] = {"/dev/sda", "/dev/sdb", "/dev/sdc", "/dev/sdd"};

for (int i = 0; i < 4; i++) {
    // 1. RAID 메타데이터 제거
    sprintf(cmd, "mdadm --zero-superblock %s 2>/dev/null", disks[i]);
    system(cmd);
    
    // 2. 파일시스템 지문 제거
    sprintf(cmd, "wipefs -a %s", disks[i]);
    system(cmd);
    
    // 3. 파티션 테이블 초기화
    sprintf(cmd, "sgdisk --zap-all %s", disks[i]);
    system(cmd);
}
```

### RAID 생성 (예: RAID 5)
```c
// 4개 디스크로 RAID 5 생성
system("mdadm --create --verbose /dev/md0 "
       "--level=5 "                    // RAID 레벨
       "--raid-devices=4 "             // 디스크 개수
       "/dev/sda /dev/sdb /dev/sdc /dev/sdd "
       "--run");                       // 즉시 시작
```

### 자동 마운트 설정
```c
// RAID 정보 저장 (부팅 시 자동 인식)
system("mdadm --detail --scan | tee -a /etc/mdadm/mdadm.conf");

// fstab에 등록 (부팅 시 자동 마운트)
system("echo '/dev/md0 /storage ext4 defaults,noatime 0 2' >> /etc/fstab");
```

---

## 🐛 주요 디버깅 사례

### Issue #1: 디스크 점유 오류
**증상:**
```bash
mdadm: Cannot open /dev/sda: Device or resource busy
```

**원인:** 이전 RAID 메타데이터가 남아있음

**해결:**
```c
// 모든 md 장치 강제 정지
system("mdadm --stop /dev/md* 2>/dev/null");

// superblock 완전 제거
system("mdadm --zero-superblock /dev/sda");
system("wipefs -a /dev/sda");
```

---

### Issue #2: 전원 부족으로 인한 I/O 에러
**증상:**
```bash
[  123.456] I/O error, dev sda, sector 1234
[  123.789] md/raid5:md0: read error
```

**원인:** Raspberry Pi 5의 USB 전원으로는 SSD 4개 구동 불가

**해결:**
![Power Adapter](img/power.webp)
```
Radxa Penta SATA HAT 전용 12V 5A 어댑터 사용 필수!
- 인터페이스: 5525 (외경 5.5mm, 내경 2.5mm)
- 출력: 12V / 5A (60W)
- 극성: 중앙 양극(+), 외부 음극(-)
```

---

### Issue #3: Samba 접속 거부
**증상:**
```
\\192.168.1.100\NAS_Storage
→ "액세스가 거부되었습니다"
```

**원인:** 방화벽(UFW)이 Samba 포트 차단

**해결:**
```c
// Samba 포트 자동 개방
system("ufw allow 137,138/udp");  // NetBIOS
system("ufw allow 139,445/tcp");  // SMB
system("ufw allow ssh");          // SSH 유지
system("ufw --force enable");
```

---

## 🌐 Windows에서 접속하기

### 방법 1: 네트워크 드라이브 연결
```
1. 파일 탐색기 열기
2. "내 PC" 우클릭 → "네트워크 드라이브 연결"
3. 폴더: \\192.168.1.100\NAS_Storage
4. 사용자 이름: pi (또는 라즈베리파이 계정명)
5. 비밀번호: [설정 시 입력한 Samba 비밀번호]
```

### 방법 2: 직접 접근
```
Windows + R → 실행
\\192.168.1.100\NAS_Storage
```

---

## ❓ FAQ

<details>
<summary><b>Q. RAID 6은 지원하나요?</b></summary>

A. 불가능합니다. Radxa Penta SATA HAT이 **하드웨어적으로 4개 디스크까지만 지원**하기 때문입니다. RAID 6은 최소 4개 디스크가 필요하지만 패리티 2개를 사용하므로 실질적으로는 불가능합니다.

HAT 제조사(Radxa) 공식 문서에서도 RAID 6 미지원을 명시하고 있습니다.
</details>

<details>
<summary><b>Q. ZFS 파일시스템을 사용할 수 있나요?</b></summary>

A. Raspberry Pi 5의 메모리(4~8GB)로는 ZFS의 성능을 제대로 낼 수 없습니다. ZFS는 최소 16GB RAM 권장입니다.
</details>

<details>
<summary><b>Q. Raspberry Pi 4에서 사용 가능한가요?</b></summary>

A. 불가능합니다. Radxa Penta SATA HAT은 Raspberry Pi 5 전용입니다 (PCIe 인터페이스 필요).
</details>

<details>
<summary><b>Q. HDD도 사용 가능한가요?</b></summary>

A. 네, SATA 인터페이스를 지원하는 2.5" HDD라면 사용 가능합니다. 다만 진동 간섭에 주의하세요.
</details>

<details>
<summary><b>Q. SSD를 2~3개만 사용하고 싶어요</b></summary>

A. 소스코드 수정이 필요합니다:
```c
// raid_main.c에서 수정
const char *disks[] = {"/dev/sda", "/dev/sdb"};  // 2개만
cleanup_disks(disks, 2);  // 4 → 2로 변경
```
</details>

<details>
<summary><b>Q. OpenMediaVault(OMV)와 함께 사용 가능한가요?</b></summary>

A. 가능합니다:
```bash
# 1. Raspberry Pi OS Lite 설치
# 2. setup_app 실행
sudo ./setup_app
# 3. 재부팅 후 OMV 설치
sudo wget -O - https://github.com/OpenMediaVault-Plugin-Developers/installScript/raw/master/install | sudo bash
```
</details>

---

## ⚠️ 중요 주의사항

### 🔴 데이터 완전 삭제
```
RAID 구성 시 연결된 모든 SSD의 데이터가 완전히 삭제됩니다!
백업 없이 실행하지 마세요!
```

### 🔴 전원 어댑터 필수
![Power Warning](img/power.webp)
```
Raspberry Pi의 USB 전원만으로는 SSD 4개 구동 불가!
반드시 Radxa Penta SATA HAT 전용 12V 5A 어댑터 사용

미사용 시 증상:
- "Input/output error"
- 디스크 인식 실패
- 시스템 불안정
```

**전원 어댑터 스펙:**
- 인터페이스: 5525 (외경 5.5mm, 내경 2.5mm)
- 입력: 100-240V ~ 50/60Hz, 최대 1.2A
- 출력: 12V / 5A (60W)
- 극성: 중앙 양극(+), 외부 음극(-)
- 인증: UL, CE, FCC

---

## 🛠️ 개발 환경

- **언어:** C (ISO C99)
- **컴파일러:** GCC 11.4+
- **개발 도구:**
  - VSCode (Windows 11) - 원격 SSH
  - vim, nano (Raspberry Pi 5)
- **테스트 환경:**
  - Raspberry Pi 5 (8GB)
  - Radxa Penta SATA HAT
  - Samsung 870 EVO 1TB SSD x 4

---

## 📈 성능 벤치마크

### 순차 읽기/쓰기 (RAID 5)
```bash
# fio 테스트 결과
Sequential Read:  450 MB/s
Sequential Write: 380 MB/s

# 단일 SSD 대비 약 3배 향상
```

### 랜덤 읽기/쓰기
```bash
Random Read (4K):  120 MB/s
Random Write (4K):  85 MB/s
```

---

## 🔮 향후 개선 계획

- [ ] 웹 대시보드 추가 (RAID 상태 모니터링)
- [ ] 이메일 알림 (디스크 장애 감지 시)
- [ ] 자동 백업 스케줄러

---

## 📚 참고 자료

- [Radxa Penta SATA HAT 공식 문서](https://docs.radxa.com/en/accessories/storage/penta-sata-hat)
- [Linux mdadm Manual](https://raid.wiki.kernel.org/index.php/RAID_setup)
- [Samba Documentation](https://www.samba.org/samba/docs/)

---

## 👨‍💻 개발자

**김진형** (System Programmer)

- 🔗 GitHub: [@epqlffltm](https://github.com/epqlffltm)
- 📧 Email: [Your Email]

---

## 📝 라이선스

MIT License - 자유롭게 사용, 수정, 배포 가능

---

## 🙏 기여

이슈 제보, 기능 제안, Pull Request는 언제나 환영합니다!

---

<p align="center">
  <strong>⭐ 이 프로젝트가 도움이 되셨다면 Star를 눌러주세요!</strong>
</p>

<p align="center">
  Made with ❤️ for Raspberry Pi enthusiasts
</p>