## 📂 프로젝트 구조
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


# 저장소 복제
```git clone https://github.com/epqlffltm/pi-nas-builder.git```

# 폴더 이동
```cd pi-nas-builder```

# 실행 파일 생성 (setup_app, raid0, raid5 등)
```make```

# 설정 파일 실행
```sudo ./setup_app```

# RAID 5 구성을 원하는 경우
```sudo ./raid5```

# 또는 상황에 따라 선택
# sudo ./raid0
# sudo ./raid10


명령어,RAID 레벨,특징,추천 용도
sudo ./raid0,RAID 0,"성능 최우선, 보호 기능 없음","단순 작업용, 임시 저장소"
sudo ./raid5,RAID 5,"안정성 + 성능, 디스크 1개 보호",일반 NAS 권장 설정
sudo ./raid10,RAID 10,최고 성능 + 최고의 안정성,중요 데이터 및 고성능 서버

⚠️ 주의사항
데이터 삭제: RAID 구성 시 연결된 모든 SSD의 데이터가 완전히 삭제되므로 실행 전 반드시 확인하십시오.

전원 공급: SSD 4개를 구동하기 위해 반드시 라즈베리 파이 5 전용 27W(5V 5A) 어댑터를 사용해야 합니다. 전력이 부족할 경우 Input/output error가 발생할 수 있습니다.
