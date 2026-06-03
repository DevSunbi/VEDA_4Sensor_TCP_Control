# 🎛️ VEDA Multi-Sensor TCP Control System

Raspberry Pi 4에서 동작하는 **TCP 멀티스레드 IoT 장치 제어 시스템**입니다.  
웹 대시보드 및 CLI 클라이언트를 통해 LED, 부저, 7세그먼트, 조도센서를 원격으로 제어하고 모니터링할 수 있습니다.

`poll()` 기반 I/O 다중화, `dlopen`/`dlsym`을 활용한 장치 드라이버 동적 로딩, `pthread_mutex` 기반 스레드 동기화, 이중 포크 데몬화 등의 시스템 프로그래밍 기법을 활용하여 구현하였습니다.

---

## 📑 문서 링크

| 문서 | 링크 |
|------|------|
| 기획서 | [TCP 원격 장치 제어 프로그램](https://www.notion.so/37388056fa6a81dab27aef8003ad44bd) |
| 개발 명세서 | [TCP 서버/클라이언트 및 장치 제어](https://www.notion.so/37388056fa6a81b79352c3b1facd4ca1) |
| 개발문서 | [프로젝트 개요, 일정, 구현 내용, 문제점 및 보완](https://www.notion.so/37388056fa6a81569352e334c032cd4b) |

---

## 📐 시스템 구조 및 네트워크 아키텍처

```
┌─────────────────────────────────────────────────────────────┐
│                       Web Browser                           │
│   (index.html / led.html / buzzer.html / segment.html / pr) │
└──────────────────────────────┬──────────────────────────────┘
                               │ HTTP Request (TCP:8000)
┌──────────────────────────────▼──────────────────────────────┐
│                    main.c (TCP Web Server)                  │
│   - poll() 멀티플렉싱 기반 동시 클라이언트 처리                │
│   - HTTP/1.1 프로토콜 분석 및 API 요청 라우팅                │
│   - 데몬화(Daemon Mode) 지원                                │
│   - 조도센서 기반 LED 백그라운드 자동 제어 스레드 구동        │
│                                                             │
│ ┌───────────────┐ ┌───────────────┐ ┌─────────────────────┐ │
│ │   libled.so   │ │  libbuzzor.so │ │    libsegment.so    │ │
│ └───────┬───────┘ └───────┬───────┘ └──────────┬──────────┘ │
│ ┌───────┴─────────────────┴────────────────────┴──────────┐ │
│ │          rpi_common.c / rpi_common.h (Shared)           │ │
│ │   - GPIO 초기화 및 공통 상태 제어 관리                  │ │
│ │   - pthread_mutex 기반 임계 구역 동기화 (락 최적화)     │ │
│ └─────────────────────────┬───────────────────────────────┘ │
│ ┌─────────────────────────▼───────────────────────────────┐ │
│ │                libphotoresistor.so (I2C)                │ │
│ │   - PCF8591 I2C 통신을 이용한 ADC 아날로그 조도 획득     │ │
│ └─────────────────────────────────────────────────────────┘ │
└──────────────────────────────┬──────────────────────────────┘
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
| CDS (AD/DA) SDA | 8 | 2 | 3 | PCF8591 ADC (0x48) |
| CDS (AD/DA) SCL | 9 | 3 | 5 | PCF8591 ADC (0x48) |
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

## 💡 주요 기능 및 구현 특징

### 1. I/O 다중화 및 멀티스레드 처리
* **`poll()` 기반 I/O 다중화**: 다수의 소켓 연결을 `poll()`로 감시하여 동시 접속 요청을 처리합니다.
* **`pthread_detach` 스레딩**: 클라이언트 요청이 수락(`accept`)되면 별도 스레드를 생성하여 처리하고, 메인 루프는 즉시 다음 연결을 수락할 수 있도록 스레드를 분리(`detach`)합니다.

### 2. 장치 드라이버 동적 로딩
* **런타임 심볼 해석 (`dlopen` / `dlsym`)**: 각 장치 제어 모듈을 공유 라이브러리(`.so`)로 분리하고, 런타임에 동적으로 로드하여 실행합니다. 서버 재컴파일 없이 개별 드라이버만 교체할 수 있습니다.

### 3. GPIO 뮤텍스 동기화
* **GPIO 경합 방지**: 여러 스레드가 동일한 GPIO 핀에 접근할 때 `pthread_mutex_t`(`gpio_lock` / `gpio_unlock`)로 임계 구역을 보호합니다.
* **락 범위 최소화**: 7세그먼트 카운트다운처럼 `delay(1000)`로 대기하는 구간에서는 뮤텍스를 해제하고, 실제 핀 출력 시점에만 락을 잡아 다른 스레드의 블로킹을 줄였습니다.

### 4. 세그먼트 카운트다운 & 취소 플래그
* **API 분리**: 카운트다운(`/segment/<digit>`)과 단발성 출력(`/segment/show/<digit>`)을 분리하여 웹 프론트엔드와 서버 간 핀 제어 충돌을 방지했습니다.
* **취소 감지**: 카운트다운 루프 매 회차마다 전역 `cancel_countdown_flag`를 확인하여, `OFF` 요청 시 즉시 중
## 📜 라이선스 및 개발자 정보다.

### 2. 빌드 실행 (Host PC)
프로젝트 루트 디렉토리에서 메이크파일을 실행합니다.
```bash
# 빌드 실행
make

# 리빌드 실행
make clean && make
```
* **결과물**: `server` (타겟 실행 파일), `client` (호스트용 CLI), `lib*.so` (동적 모듈 라이브러리들)

### 3. 파일 배포 (Host PC → Raspberry Pi)
빌드된 빌드 바이너리와 HTML 리소스 폴더를 타겟 장치로 동기화합니다.
```bash
# 바이너리 파일 전송
scp server lib*.so sunbi@<RPI_IP>:/home/sunbi/projectHome/

# 리소스 폴더 재귀 전송
scp -r resources/ sunbi@<RPI_IP>:/home/sunbi/projectHome/
```

### 4. 서버 구동 및 제어

**라즈베리 파이 (서버 실행):**
```bash
cd /home/sunbi/projectHome
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

# 1. 백그라운드 데몬 프로세스로 상시 구동
./server

# 2. 디버깅 모드(포그라운드 로그 출력)로 구동
./server -f
```

**호스트 PC (제어 및 클라이언트 실행):**
* **CLI 원격 제어 콘솔**:
  ```bash
  ./client <RPI_IP>
  ```
* **웹 대시보드 원격 접속**:
  웹 브라우저 주소창에 `http://<RPI_IP>:8000/` 접속

---

## 🐛 이슈 트래킹

프로젝트 개발 중 발견된 버그 및 개선 사항은 [GitHub Issues](https://github.com/DevSunbi/VEDA_4Sensor_TCP_Control/issues)에서 관리합니다.

* **해결 완료(Closed)**: GPIO 핀 매핑, 라우팅 충돌, 카운트다운 동기화, 데몬 경로 해석 등 20건 이상의 이슈를 식별 및 해결하였습니다.
* **진행 중(Open)**: 현재 오픈된 이슈 목록은 [Issues 탭](https://github.com/DevSunbi/VEDA_4Sensor_TCP_Control/issues?q=is%3Aissue+state%3Aopen)에서 확인할 수 있습니다.


## 📜 라이선스 및 개발자 정보
* **개발자**: sunbi (VEDA 4기 멤버)
* **목적**: Raspberry Pi 4 환경에서의 TCP 통신 및 하드웨어 멀티 스레드 드라이빙 학습용 프로젝트
* **라이선스**: MIT License
