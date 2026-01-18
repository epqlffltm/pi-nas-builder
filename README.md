# Radxa Penta SATA HAT pi-nas builder

![hat](/img/hat.webp)
---
## [Radxa Penta SATA HAT](https://docs.radxa.com/en/accessories/storage/penta-sata-hat)을 사용한 자동화 설정 빌드입니다. 

---
## 프로젝트 목표

Radxa Penta SATA HAT을 만들떄 일일히 리눅스 명령어를 치기 귀찮고, 설정을 만들기 귀찮아서 만들었습니다.

---
## 재료
1. raspbarry pi 5
2. SD card
3. Radxa Penta SATA HAT
4. sata 2.5inch ssd 4장

---
## 📂 프로젝트 구조
```
.
├── Makefile           # 전체 빌드 스크립트
├── README.md          # 프로젝트 설명서
├── setup/             # 단계 1: 시스템 초기 환경 설정 소스
│   └── setup.c
├── install/           # 단계 2: RAID 구성 메인 소스
│   └── raid_main.c
└── lib/               # 공통 라이브러리 및 유틸리티
    ├── nas_lib.c
    └── nas_lib.h
```
---
### 저장소 복제

```
git clone https://github.com/epqlffltm/pi-nas-builder.git
```

---
### 폴더 이동

```
cd pi-nas-builder
```
---
### 실행 파일 생성 (setup_app, raid0, raid5 등)

```
make
```
---
### 설정 파일 실행

```
sudo ./setup_app
```
---
### RAID 0 구성을 원하는 경우

```
sudo ./raid0
```
---
### RAID 1 구성을 원하는 경우

```
sudo ./raid1
```
---
### RAID 5 구성을 원하는 경우

```
sudo ./raid5
```
---
### RAID 10 구성을 원하는 경우

```
sudo ./raid10
```
---


| RAID Level | 특징 | 추천 | 용도 |
| --- | --- | --- | --- |
| RAID 0 | 성능 최우선 | 보호 기능 없음 | 단순 작업용, 임시 저장소 |
| RAID 5 | 안정성 + 성능 | 디스크 1개 보호 | 일반 NAS 권장 설정 |
| RAID 10 | 최고 성능 + 최고의 안정성 | 중요 데이터 및 고성능 서버 |

---
### 연결 방법
1. 윈도우 내PC에서 네트워크 위치 추가
2.  \\(나의 raspbarry pi ip)\NAS_Storage 입력
3. 나의 raspbarry pi id와 비밀번호 입력

---
### 개발환경
C 컴파일러: gcc (WSL/리눅스 환경 기준)

---
### QnA
Q. RAID 6은 안되나요?

A. Radxa Penta SATA HAT에서 지원하지 않습니다. 저도 시도는 해보았지만 실패했습니다.



Q. ZFS는 사용이 불가능한가요?

A. raspbarrty pi의 한계상 제 성능은 나오지 않을 것 같습니다.
   정 하시려면 최소 16gb 램에서 하시길 바랍니다.



Q. 라즈베리 4는 사용이 불가능한가요?

A. 예, hat 자체가 5와만 호환됩니다.



Q. rock pi에서도 사용이 가능한가요?

A. 해본적이 없어서 잘 모르겠습니다.



Q. SSD대신 HDD도 사용 가능한가요?

A. 네, 간섭만 없다면 사용이 가능합니다.



Q. SSD를 4장 이하나 5장을 연결하고 싶다면 어떻게 해야 하나요?

A. 코드를 수정해야 합니다.



Q. OMV를 써도 되나요?

A. 해보지는 않았지만 라즈비안 os는 lite버전을 설치하시고 sudo ./setup_app까지 하신 후 설치하면 될 겁니다.




# ⚠️ 주의사항
데이터 삭제: RAID 구성 시 연결된 모든 SSD의 데이터가 완전히 삭제되므로 실행 전 반드시 확인하십시오.

![power](/img/power.webp)

전원 공급: SSD 4개를 구동하기 위해 반드시 Radxa Penta SATA HAT의 power jack 어댑터를 사용해야 합니다. 전력이 부족할 경우 Input/output error가 발생할 수 있습니다.

인터페이스: 5525, 즉 외경 5.5mm, 내경 2.5mm입니다.

입력: 100-240V ~ 50/60Hz, 최대 1.2A

출력: 12V / 5A 60W

인증: UL, CE, FCC 등

극성: 중앙 양극, 외부 음극



