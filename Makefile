CC = gcc
CFLAGS = -Wall -O2 -Ilib  # lib 폴더를 헤더 탐색 경로에 추가

# 소스 파일 경로 설정
LIB_SRC = lib/nas_lib.c
SETUP_SRC = setup/setup.c
RAID_SRC = install/raid_main.c

# 생성할 타겟 목록
TARGETS = setup_app raid0 raid1 raid5 raid10

all: $(TARGETS)

# setup_app 빌드
setup_app: $(SETUP_SRC) $(LIB_SRC)
	$(CC) $(CFLAGS) -o setup_app $(SETUP_SRC) $(LIB_SRC)

# RAID 레벨별 빌드 (매크로 사용)
raid0: $(RAID_SRC) $(LIB_SRC)
	$(CC) $(CFLAGS) -DRAID_LEVEL=0 -o raid0 $(RAID_SRC) $(LIB_SRC)

raid1: $(RAID_SRC) $(LIB_SRC)
	$(CC) $(CFLAGS) -DRAID_LEVEL=1 -o raid1 $(RAID_SRC) $(LIB_SRC)

raid5: $(RAID_SRC) $(LIB_SRC)
	$(CC) $(CFLAGS) -DRAID_LEVEL=5 -o raid5 $(RAID_SRC) $(LIB_SRC)

raid10: $(RAID_SRC) $(LIB_SRC)
	$(CC) $(CFLAGS) -DRAID_LEVEL=10 -o raid10 $(RAID_SRC) $(LIB_SRC)

clean:
	rm -f $(TARGETS)