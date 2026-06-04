# VEDA Multi-Sensor TCP Control System

Raspberry Pi 4에서 동작하는 TCP 기반 멀티스레드 IoT 장치 제어 시스템입니다.
웹 대시보드와 CLI 클라이언트를 통해 LED, 부저, 7세그먼트, 조도센서를 원격으로 제어하고 모니터링합니다.

이 프로젝트는 `poll()` 기반 I/O 다중화, `pthread` 기반 요청 처리, `dlopen`/`dlsym` 기반 장치 드라이버 동적 로딩, GPIO 공유 상태 동기화, 데몬 실행을 학습하기 위한 시스템 프로그래밍 프로젝트입니다.

---

## 제출 문서

| 제출물 | 파일 | 내용 |
|--------|------|------|
| 개발문서 | [개발문서.md](./개발문서.md) | 프로젝트 개요, 개발 일정, 세부 구현 내용, 문제점 및 보완 사항 |
| README | [README.md](./README.md) | 소스 구조, 빌드 방법, 배포 및 실행 방법 |
| 실행 과정 text 파일 | [실행_과정.txt](./실행_과정.txt) | 빌드, 배포, 서버 실행, 웹/CLI 장치별 시연 순서 |

---

## 평가 기준 대응 요약

| 평가 항목 | 대응 내용 |
|-----------|-----------|
| LED 제어 | `/led/on`, `/led/mid`, `/led/off`, `/led/brightness?value=0..1024` 및 CLI LED 메뉴 |
| 부저 제어 | `/buzz/do` 등 계이름, 숫자 음계, 직접 주파수, `/buzz/off`, `/buzz/fein` |
| 조도센서 | `/pr` 아날로그 ADC 조회, `/pr/digital` 디지털 조회, `/pr/auto/start` 자동 LED 제어 |
| 7세그먼트 | `/segment/start`, `/segment/<0..9>`, `/segment/show/<0..9>`, `/segment/off` |
| 구현 내용 | `poll()` 연결 감시, 요청별 `pthread_detach`, 동적 라이브러리 분리, GPIO 뮤텍스, 데몬 모드 |
| 사용자 편의성 | 웹 대시보드와 CLI 클라이언트 병행 제공, `-f` 전경 디버그 모드, SIGINT 종료 처리 |
| 문서 | README, 개발문서, 실행 과정 text 파일 작성 |
| 추가 기능 | PWM 밝기 제어, FEIN 알림 멜로디, 조도 기반 서버 자동 LED 제어, GitHub Issues 기반 문제 추적 |

---

## 문서 링크

| 문서 | 링크 |
|------|------|
| 기획서 | [TCP 원격 장치 제어 프로그램](https://www.notion.so/37388056fa6a81dab27aef8003ad44bd) |
| 개발 명세서 | [TCP 서버/클라이언트 및 장치 제어](https://www.notion.so/37388056fa6a81b79352c3b1facd4ca1) |
| 개발문서 | [프로젝트 개요, 일정, 구현 내용, 문제점 및 보완](https://www.notion.so/37388056fa6a81569352e334c032cd4b) |

---

## 시스템 구조

```text
Web Browser / CLI Client
        |
        | HTTP-like TCP request (:8000)
        v
src/main.c
  - poll() 기반 연결 감시
  - 요청별 detached pthread 처리
  - 정적 HTML 리소스 제공
  - 장치 제어 API 라우팅
  - 조도센서 기반 자동 LED 제어 스레드
        |
        +-- libled.so
        +-- libbuzzor.so
        +-- libsegment.so
        +-- libphotoresistor.so
        |
        v
lib/rpi_common.c / lib/rpi_common.h
  - wiringPi 초기화
  - GPIO 뮤텍스
  - LED 상태 및 카운트다운 취소 플래그 공유
        |
        v
Raspberry Pi 4 GPIO / I2C
```

공유 라이브러리 파일명은 현재 코드와 동일하게 `libbuzzor.so`를 사용합니다.

---

## 회로 구성도

![Raspberry Pi 4 Sensor and Output Circuit](./docs/images/raspberry-pi-4-sensor-output-circuit.png)

실제 핀 연결 및 세그먼트 타입은 아래 핀맵과 코드 설정(`lib/rpi_common.h`, `lib/segment.c`)을 우선합니다.

---

## 주요 기능

| 기능 | 설명 |
|------|------|
| 웹 대시보드 | `resources/` 아래 HTML 페이지로 LED, 부저, 7세그먼트, 조도센서를 제어 |
| CLI 클라이언트 | `client <RPI_IP>`로 서버에 접속해 장치 제어 명령 전송 |
| LED 제어 | ON, MID, OFF 및 `0~1024` PWM 밝기 제어 |
| 부저 제어 | 계이름, 숫자 음계, 직접 주파수, 알림 멜로디 제어 |
| 7세그먼트 | 카운트다운, 단일 숫자 표시, OFF 요청 기반 취소 처리 |
| 조도센서 | PCF8591 I2C ADC 값 조회, 디지털 조도 값 조회, 자동 LED 제어 |
| 데몬 모드 | 기본 실행 시 백그라운드 데몬으로 실행, `-f` 또는 `--foreground`로 전경 실행 |

---

## API 경로

| 경로 | 동작 |
|------|------|
| `/` | `resources/index.html` 제공 |
| `/led/on` | LED 최대 밝기 ON |
| `/led/mid` | LED 중간 밝기 |
| `/led/off` | LED OFF 및 카운트다운 취소 플래그 설정 |
| `/led/brightness?value=0..1024` | PWM 밝기 제어 |
| `/led/state` | 서버가 추적 중인 LED 상태 조회 |
| `/buzz/do`, `/buzz/re`, ... | 부저 계이름 재생 |
| `/buzz/1` ~ `/buzz/8` | 부저 숫자 음계 재생 |
| `/buzz/<frequency>` | 직접 주파수 재생 |
| `/buzz/off` | 부저 정지 |
| `/buzz/fein` 또는 `/buzz/alert` | 알림 멜로디 재생 |
| `/segment/start` | 9부터 0까지 카운트다운 |
| `/segment/<0..9>` | 지정 숫자부터 0까지 카운트다운 |
| `/segment/show/<0..9>` | 지정 숫자 단발 표시 |
| `/segment/off` | 7세그먼트 및 LED OFF |
| `/ldr/start` | `/segment/start`와 동일한 호환 경로 |
| `/pr` | PCF8591 ADC 아날로그 조도 값 조회 |
| `/pr/digital` | 디지털 조도 값 조회 |
| `/pr/auto/start` | 서버 측 자동 LED 제어 시작 |
| `/pr/auto/stop` | 서버 측 자동 LED 제어 중지 |

---

## 조도센서 동작 기준

아날로그 조도 값은 `lib/photoresistor.c`의 `get_cds_value()`가 PCF8591 I2C ADC에서 읽어 옵니다.

| 값 | 기준 |
|----|------|
| ADC 값 | `0~255` 범위의 아날로그 조도 값 |
| 기본 임계값 | `threshold = 200` |
| 밝음 판단 | ADC 값이 임계값보다 작을 때 |
| 어두움 판단 | ADC 값이 임계값 이상일 때 |

디지털 조도 값은 `get_pr_value()`가 `digitalRead(PR_PIN)`으로 읽습니다.
서버 자동 LED 제어는 현재 디지털 값을 기준으로 동작합니다.

| 디지털 값 | 의미 | 자동 제어 |
|-----------|------|-----------|
| `0` / `LOW` | 어두움 | LED ON |
| `1` / `HIGH` | 밝음 | LED OFF |

---

## 핀맵

| 장치 | wPi Pin | BCM GPIO | Physical Pin | 비고 |
|------|---------|----------|--------------|------|
| LED | 1 | 18 | 12 | PWM, 220Ω |
| Buzzer | 2 | 27 | 13 | softTone |
| CDS SDA | 8 | 2 | 3 | PCF8591 ADC, I2C `0x48` |
| CDS SCL | 9 | 3 | 5 | PCF8591 ADC, I2C `0x48` |
| CDS Digital | 0 | 17 | 11 | Pull-up, 10kΩ |
| 7-Segment A | 21 | 5 | 29 | Common Anode |
| 7-Segment B | 22 | 6 | 31 | Common Anode |
| 7-Segment C | 23 | 13 | 33 | Common Anode |
| 7-Segment D | 24 | 19 | 35 | Common Anode |
| 7-Segment E | 25 | 26 | 37 | Common Anode |
| 7-Segment F | 27 | 16 | 36 | Common Anode |
| 7-Segment G | 28 | 20 | 38 | Common Anode |
| 7-Segment DP | 29 | 21 | 40 | Common Anode |
| Default VCC | - | - | 1 | 3.3V |
| 7-Segment VCC | - | - | 2 | 5V |
| Default GND | - | - | 9 | GND |

---

## 빌드 환경

`Makefile`은 서버와 공유 라이브러리를 Raspberry Pi용 AArch64 바이너리로 크로스 컴파일하고, CLI 클라이언트는 호스트 PC용 `gcc`로 빌드합니다.

필요한 도구와 경로:

```bash
aarch64-linux-gnu-gcc
gcc
make
~/rpi-sysroot
~/rpi-sysroot/usr/local/include
~/rpi-sysroot/usr/local/lib
```

`~/rpi-sysroot`에는 Raspberry Pi의 wiringPi 헤더와 라이브러리가 포함되어 있어야 합니다.

---

## 빌드

프로젝트 루트에서 실행합니다.

```bash
make
```

리빌드가 필요하면 다음 명령을 사용합니다.

```bash
make clean && make
```

빌드 결과:

| 파일 | 설명 |
|------|------|
| `server` | Raspberry Pi에서 실행할 TCP 웹 서버 |
| `client` | 호스트 PC에서 실행할 CLI 클라이언트 |
| `libled.so` | LED 제어 드라이버 |
| `libbuzzor.so` | 부저 제어 드라이버 |
| `libphotoresistor.so` | 조도센서 드라이버 |
| `libsegment.so` | 7세그먼트 드라이버 |

---

## 배포

배포 기준 경로는 `/home/sunbi/prj`로 통일합니다.
서버는 데몬 실행 시 `/proc/self/exe`를 통해 실행 파일이 있는 디렉토리로 이동하므로, 실행 파일과 `lib*.so`, `resources/`가 같은 디렉토리 아래에 있으면 됩니다.

```bash
ssh sunbi@<RPI_IP> "mkdir -p /home/sunbi/prj"

scp server lib*.so sunbi@<RPI_IP>:/home/sunbi/prj/
scp -r resources sunbi@<RPI_IP>:/home/sunbi/prj/
```

---

## 실행

Raspberry Pi에서 서버를 실행합니다.

```bash
cd /home/sunbi/prj
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
./server
```

전경 로그를 보면서 디버깅하려면 다음 명령을 사용합니다.

```bash
cd /home/sunbi/prj
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
./server -f
```

호스트 PC에서 CLI 클라이언트를 실행합니다.

```bash
./client <RPI_IP>
```

웹 대시보드는 브라우저에서 다음 주소로 접속합니다.

```text
http://<RPI_IP>:8000/
```

---

## 이슈 트래킹

프로젝트 개발 중 발견된 버그와 개선 사항은 [GitHub Issues](https://github.com/DevSunbi/VEDA_4Sensor_TCP_Control/issues)에서 관리합니다.

2026-06-04 KST 기준 확인된 상태:

| 상태 | 건수 | 비고 |
|------|------|------|
| Open | 2 | #12 최종 체크리스트, #20 클라이언트 연결 실패 재시도 UX |
| Closed | 18 | GPIO 핀 매핑, 라우팅 충돌, 카운트다운 동기화, 데몬 경로 해석 등 |
| Total | 20 | GitHub Issues 기준 |

현재 남은 주요 보완 과제:

| 이슈 | 내용 |
|------|------|
| [#12](https://github.com/DevSunbi/VEDA_4Sensor_TCP_Control/issues/12) | 문서 경로 정합성, 조도센서 기준 명세, 실행 과정 파일, 개발문서 보완 |
| [#20](https://github.com/DevSunbi/VEDA_4Sensor_TCP_Control/issues/20) | `client.c` 서버 접속 실패 시 재시도 백오프 및 메뉴 복구 |

---

## 알려진 보완 지점

| 항목 | 설명 |
|------|------|
| 클라이언트 연결 실패 UX | 서버 IP 오류나 네트워크 단절 시 `connect()` 실패 후 즉시 반환하므로 재시도 백오프가 필요합니다. |
| 조도센서 자동 제어 종료성 | `cdsControl()`은 무한 루프 구조이므로 서버 자동 제어는 `get_pr_value()`를 주기 호출하는 방식으로 대체되어 있습니다. |
| 카운트다운 정밀도 | `delay(1000)`과 GPIO 뮤텍스 재획득 타이밍에 따라 지터가 생길 수 있습니다. |
| 라이선스 파일 | README에는 MIT License로 표기되어 있으나, 별도 `LICENSE` 파일 추가를 권장합니다. |

---

## 라이선스 및 개발자 정보

| 항목 | 내용 |
|------|------|
| 개발자 | sunbi, VEDA 4기 |
| 목적 | Raspberry Pi 4 환경에서 TCP 통신, 동적 라이브러리 로딩, 멀티스레드 GPIO 제어 학습 |
| 라이선스 | MIT License |
