#!/bin/bash
# setup.sh
# 에러 발생시 중단
set -e

echo "라즈베리파이 ZFS(Stripe) 셋업 시작..."

# 1. 시스템 패키지 업데이트
echo "시스템 패키지 업데이트 중..."
sudo apt update && sudo apt upgrade -y

# 1-1. 펌웨어(EEPROM) 업데이트 (중요)
# PCIe 호환성 및 안정성을 위해 최신 펌웨어 적용
echo "라즈베리파이 펌웨어(EEPROM) 업데이트 준비 중..."
sudo apt install rpi-eeprom -y
# -a 옵션: 업데이트가 있으면 자동으로 적용 예약 (재부팅 시 실제 적용됨)
sudo rpi-eeprom-update -a

# 2. 커널 헤더 설치
echo "커널 헤더 설치 중 (시간 소요됨)..."
sudo apt install linux-headers-$(uname -r)

# 3. PCIe 설정
# 주의: Pi 5에서 Gen 3 사용을 위해 설정
CONFIG_FILE="/boot/firmware/config.txt"
echo "PCIe Gen 3 설정 확인 중..."
if ! grep -q "dtparam=pciex1" "$CONFIG_FILE"; then
    echo "dtparam=pciex1" | sudo tee -a "$CONFIG_FILE"
fi
if ! grep -q "dtparam=pciex1_gen=3" "$CONFIG_FILE"; then
    echo "dtparam=pciex1_gen=3" | sudo tee -a "$CONFIG_FILE"
fi

# 4. ZFS 및 필수 패키지 설치
echo "ZFS 및 필수 패키지 설치 중..."
echo "경고: ZFS 모듈 컴파일로 인해 시간이 오래 걸립니다 (5~15분)."
sudo apt install vim ufw samba -y
sudo apt install zfs-dkms zfsutils-linux -y

# 5. 방화벽 설정
echo "방화벽 설정 중..."
sudo ufw allow ssh
sudo ufw allow samba
sudo ufw allow 5000/tcp
sudo ufw --force enable

echo "셋업 완료! 펌웨어 적용 및 ZFS 모듈 로드를 위해 재부팅합니다..."
# 이때 펌웨어 업데이트가 진행되므로 부팅이 평소보다 몇 초 더 걸릴 수 있습니다.
sudo reboot