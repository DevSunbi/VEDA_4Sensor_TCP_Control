# 🎛️ VEDA 4-Sensor TCP Control

Raspberry Pi 4에서 동작하는 **TCP 기반 IoT 장치 원격 제어 시스템**입니다.  
웹 브라우저를 통해 LED, 부저, 7세그먼트, 조도센서를 실시간으로 제어하고 모니터링할 수 있습니다.

## 📑 문서 링크

| 문서 | 링크 |
|------|------|
| 기획서 | [TCP 원격 장치 제어 프로그램](https://www.notion.so/37388056fa6a81dab27aef8003ad44bd) |
| 개발 명세서 | [TCP 서버/클라이언트 및 장치 제어](https://www.notion.so/37388056fa6a81b79352c3b1facd4ca1) |
| 개발문서 | [프로젝트 개요, 일정, 구현 내용, 문제점 및 보완](https://www.notion.so/37388056fa6a81569352e334c032cd4b) |

---

## 📐 시스템 구조

```
┌─────────────────────────────────────────────────┐
│                  Web Browser                    │
│    index.html / led.html / buzzer.html / ...    │
└──────────────────────┬──────────────────────────┘
                       │ HTTP (TCP:8000)
┌──────────────────────▼──────────────────────────┐
│              main.c  (TCP Server)                │
│         요청 파싱 → 라우팅 → 응답 반환           │
│                                                  │
│  ┌────────────┐ ┌────────────┐ ┌──────────────┐ │
│  │ libled.so  │ │libbuzzor.so│ │libsegment.so │ │
│  └─────┬──────┘ └─────┬──────┘ └──────┬───────┘ │
│  ┌─────┴──────────────┴───────────────┴───────┐ │
│  │         rpi_common.c / rpi_common.h        │ │
│  │     GPIO 초기화 · 뮤텍스 · 상태 관리        │ │
│  └────────────────────┬───────────────────────┘ │
│  ┌────────────────────┴───────────────────────┐ │
│  │         libphotoresistor.so (I2C)          │ │
│  └────────────────────────────────────────────┘ │
└──────────────────────┬──────────────────────────┘
                       │ wiringPi GPIO / I2C
              ┌────────▼────────┐
              │  Raspberry Pi 4 │
              │   GPIO Header   │
              └─────────────────┘
```

---

## ⚙️ 핀맵 (wiringPi 기준)

| 장치 | wPi Pin | BCM GPIO | Physical Pin | 비고 |
|------|---------|----------|--------------|------|
| LED | 1 | 18 | 12 | 220Ω |
| Buzzer | 2 | 27 | 13 | - |
| CDS (AD/DA) | - | - | I2C | PCF8591 ADC (0x48) |
| CDS (Analog) | 0 | 17 | 11 | Pull Up, 10kΩ |
| 7-Segment A | 21 | 5 | 29 | - |
| 7-Segment B | 22 | 6 | 31 | - |
| 7-Segment C | 23 | 13 | 33 | - |
| 7-Segment D | 24 | 19 | 35 | - |
| 7-Segment E | 25 | 26 | 37 | - |
| 7-Segment F | 27 | 16 | 36 | - |
| 7-Segment G | 28 | 20 | 38 | - |
| 7-Segment DP | 29 | 21 | 40 | - |
| 7-Segment VCC | - | - | 2 | 330kΩ * 2 (5V) |
| Default VCC | - | - | 1 | Public VCC(3.3V) |
| Default GND | - | - | 9 | Public GND |

> **참고**: 7세그먼트는 Common Anode 타입으로 구현되어 있습니다 (`COMMON_ANODE 1`).

---

## 🔧 빌드 환경

### 사전 요구사항

| 항목 | 요구사항 |
|------|----------|
| 호스트 OS | Ubuntu Linux (WSL 포함) |
| 크로스 컴파일러 | `aarch64-linux-gnu-gcc` |
| 타겟 Sysroot | `~/rpi-sysroot` (Raspberry Pi OS 파일시스템 복사본) |
| 타겟 보드 | Raspberry Pi 4 (aarch64) |
| 라이브러리 | wiringPi (타겟에 설치 필요) |

### 크로스 컴파일러 설치 (Ubuntu)

```bash
sudo apt update
sudo apt install gcc-aarch64-linux-gnu
```

---

## 🏗️ 빌드 방법

### 1. 전체 빌드

```bash
cd ~/src
make
```

빌드 결과물:

| 파일 | 설명 |
|------|------|
| `server` | TCP 웹 서버 실행 파일 |
| `libled.so` | LED 제어 공유 라이브러리 |
| `libbuzzor.so` | 부저 제어 공유 라이브러리 |
| `libsegment.so` | 7세그먼트 제어 공유 라이브러리 |
| `libphotoresistor.so` | 조도센서 제어 공유 라이브러리 |

### 2. 클린 빌드

```bash
make clean && make
```

---

## 🚀 배포 및 실행

### 1. 파일 전송 (호스트 → Raspberry Pi)

```bash
# 실행 파일 및 공유 라이브러리 전송
scp server lib*.so sunbi@<RPI_IP>:/home/sunbi/projectHome/

# HTML 프론트엔드 파일 전송
scp src/*.html sunbi@<RPI_IP>:/home/sunbi/projectHome/src/
```

### 2. 서버 실행 (Raspberry Pi)

```bash
cd /home/sunbi/projectHome
./server
```

서버가 정상적으로 시작되면 아래 메시지가 출력됩니다:

```
GPIO initialized successfully
Server started on port 8000...
```

### 3. 웹 브라우저 접속

```
http://<RPI_IP>:8000/
```

---

## 🌐 API 라우팅

| 엔드포인트 | 메서드 | 설명 |
|-----------|--------|------|
| `/` | GET | 메인 대시보드 (index.html) |
| `/index.html` | GET | 메인 대시보드 |
| `/led.html` | GET | LED 제어 페이지 |
| `/buzzer.html` | GET | 부저 제어 페이지 |
| `/segment.html` | GET | 7세그먼트 카운트다운 페이지 |
| `/pr.html` | GET | 조도센서 모니터 페이지 |
| `/led/on` | GET | LED 켜기 |
| `/led/off` | GET | LED 끄기 |
| `/led/state` | GET | LED 상태 조회 (`ON` / `OFF`) |
| `/buzz/<note>` | GET | 부저 음계 재생 (`do`, `re`, `mi`, `fa`, `sol`, `la`, `si`, `do2`, `off`) |
| `/segment/start` 또는 `/ldr/start` | GET | 9→0 카운트다운 시작 |
| `/segment/off` | GET | 카운트다운 중단 및 세그먼트/LED 끄기 |
| `/pr` | GET | 조도센서 ADC 값 조회 (0~255) |

---

## 📁 디렉터리 구조

```
src/
├── Makefile                    # 크로스 컴파일 빌드 스크립트
├── README.md                   # 프로젝트 문서
├── lib/
│   ├── rpi_common.h            # 공통 핀 정의 및 함수 선언
│   ├── rpi_common.c            # GPIO 초기화, 뮤텍스, 상태 관리
│   ├── led.c                   # LED ON/OFF 제어
│   ├── buzzor.c                # 부저 음계 재생 (softTone)
│   ├── segment.c               # 7세그먼트 카운트다운 표시
│   └── photoresistor.c         # 조도센서 I2C ADC 읽기
├── src/
│   ├── main.c                  # TCP 웹 서버 (라우팅, 스레드 처리)
│   ├── index.html              # 메인 대시보드
│   ├── led.html                # LED 제어 UI
│   ├── buzzer.html             # 부저 피아노 키보드 UI
│   ├── segment.html            # 7세그먼트 카운트다운 UI
│   └── pr.html                 # 조도센서 실시간 모니터 UI
├── server                      # (빌드 산출물) 실행 파일
├── libled.so                   # (빌드 산출물) LED 라이브러리
├── libbuzzor.so                # (빌드 산출물) 부저 라이브러리
├── libsegment.so               # (빌드 산출물) 세그먼트 라이브러리
└── libphotoresistor.so         # (빌드 산출물) 조도센서 라이브러리
```

---

## 🔑 핵심 설계

### 동적 라이브러리 로딩

서버는 장치 제어 기능을 직접 링크하지 않고, 요청 시 `dlopen` → `dlsym` → 함수 호출 → `dlclose` 방식으로 런타임에 동적 로드합니다. 이를 통해 라이브러리만 교체하면 서버 재컴파일 없이 기능을 업데이트할 수 있습니다.

### 공유 상태 관리

동적 라이브러리는 매 요청마다 로드/언로드되므로, LED 상태 등의 정보를 `rpi_common.c`의 전역 변수(`shared_led_state`)로 관리합니다. 서버 본체(`main.c`)에 정적 링크된 `rpi_common.c`가 모든 라이브러리 간의 상태 동기화를 담당합니다.

### GPIO 뮤텍스

멀티스레드 환경에서 GPIO 핀 충돌을 방지하기 위해 `pthread_mutex`를 사용합니다. 장시간 실행되는 세그먼트 카운트다운은 `delay()` 구간에서 뮤텍스를 해제하여 다른 장치가 즉시 응답할 수 있도록 합니다.

### 카운트다운 취소

7세그먼트 카운트다운(10초) 진행 중 OFF 요청이 들어오면 `cancel_countdown_flag`를 통해 루프를 즉시 중단합니다.

---

## 🐛 해결된 이슈

| # | 이슈 | 원인 | 해결 |
|---|------|------|------|
| 1 | HTML 파일 404 에러 | Pi에 `src/` 디렉토리 미전송 | `scp`로 HTML 파일 전송 |
| 2 | LED 물리적으로 미동작 | `LED_PIN`이 실제 연결 핀과 불일치 | `wPi 1`로 수정 |
| 3 | LED 상태 조회 부정확 | `digitalRead`로 출력 핀 읽기 시 불안정 | 메모리 기반 상태 추적으로 전환 |
| 4 | 카운트다운 중 OFF 무시 | 10초 루프 진행 중 다른 요청 차단 | 취소 플래그 + 루프 내 체크 |
| 5 | `/pr.html` 접속 시 숫자만 출력 | `strncmp("pr", 2)` 라우팅 충돌 | `strcmp("pr")` 정확 매칭으로 수정 |

---

## 📜 라이선스

본 프로젝트는 학습 목적으로 제작되었습니다.
