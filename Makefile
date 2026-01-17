CC=gcc
CFLAGS=-Wall -O2 -Ilib
# 실행 파일 이름 설정
SETUP_TARGET=setup_app
INSTALL_TARGET=install_app

# 공통 소스
LIB_SRC=lib/utils.c

all: $(SETUP_TARGET) $(INSTALL_TARGET)

# setup 프로그램 빌드
$(SETUP_TARGET): setup/setup.c $(LIB_SRC)
	$(CC) $(CFLAGS) -o $(SETUP_TARGET) setup/setup.c $(LIB_SRC)

# install 프로그램 빌드
$(INSTALL_TARGET): install/install.c $(LIB_SRC)
	$(CC) $(CFLAGS) -o $(INSTALL_TARGET) install/install.c $(LIB_SRC)

clean:
	rm -f $(SETUP_TARGET) $(INSTALL_TARGET)