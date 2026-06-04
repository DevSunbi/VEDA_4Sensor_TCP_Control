# [Bug/Refactor] client.c 7-segment 명령 검증 누락 및 SIGINT 핸들러 컴파일 에러 해결

## 1. 이슈 개요
`client.c` 프로그램에서 발생하던 두 가지 주요 문제를 식별하고 해결했습니다.
1. **7-segment 제어 명령어 검증 누락**: 사용자 입력값이 서버 규격에 맞는지 검증하지 않고 전송하여 비정상 패킷이 전송될 수 있었던 문제.
2. **SIGINT 핸들러 컴파일 에러**: `signal(SIGINT, sigint_handler)` 코드는 활성화되어 있으나, 핸들러 함수 자체는 주석 처리되어 빌드가 실패하던 문제.

추가로, **제출 전 확인 메모**의 요구사항을 충족하기 위해 POSIX 비동기 시그널 안전성 준수, 입력 자릿수 1자리 검증 강화, 서버 연결 실패 백오프 재시도 로직을 보완했습니다.

---

## 2. 세부 문제 상황 및 요구사항 반영

### ① 7-segment 명령 검증 부재 & 1자리 해석 준수
- **현상**: choice == 3 분기에서 입력 검증이 없어 비정상 패킷이 전송될 수 있었음. 또한 단일 1자리 장치이므로 `00`과 같은 입력을 차단할 필요가 있었음.
- **수정**: `start`, `off` 명령어 허용 및 숫자 타입 입력(`0-9`, `show/0-9`)에 대해 자릿수 제한(`strlen == 1`)을 추가하여 `00` 입력을 원천 차단하고 `0` 또는 `show/0` 기준의 1자리 입력만 안전하게 수용함.

### ② SIGINT 핸들러 컴파일 에러 & POSIX 비동기 안전성 개선
- **현상**: 기존의 `sigint_handler`는 `printf`, `send_command` 등 비동기 시그널 안전하지 않은(Async-Signal-Safe) 함수를 시그널 컨텍스트 내부에서 호출하는 구조였습니다.
- **수정**: 비동기 안전성을 극대화하기 위해 `sigaction()` + `volatile sig_atomic_t server_running` 플래그 방식을 적용했습니다.
  - Ctrl+C 발생 시 시그널 핸들러는 안전하게 `server_running = 0` 플래그만 설정합니다.
  - 메인 스레드의 입력 대기(`fgets`)가 인터럽트(`EINTR`)되거나 루프 검사 시점에 안전하게 메인 루프를 탈출한 후, 메인 스레드 컨텍스트에서 안전하게 서버 중지 요청 전송 및 리소스를 정리하고 프로그램을 종료합니다.

### ③ 연결 실패 UX 개선 (재시도 백오프 적용)
- **현상**: 기존에는 서버 접속 실패 시 단 1회 실패 메시지만 출력하고 즉시 리턴함.
- **수정**: `send_command` 함수에서 `connect` 실패 시 최대 3회까지 **지수 백오프(Exponential Backoff)** 방식으로 지연 시간을 늘려가며(1초, 2초, 4초...) 재시도를 수행하도록 보완했습니다.

---

## 3. 변경 및 해결 내용

### ① `choice == 3` 입력 검증 코드
```c
int valid = 0;
if (strcasecmp(seg_cmd, "start") == 0 || strcasecmp(seg_cmd, "off") == 0) {
    valid = 1;
} else if (is_numeric(seg_cmd) && strlen(seg_cmd) == 1) {
    int val = atoi(seg_cmd);
    if (val >= 0 && val <= 9) valid = 1;
} else if (strncmp(seg_cmd, "show/", 5) == 0) {
    char *val_str = seg_cmd + 5;
    if (is_numeric(val_str) && strlen(val_str) == 1) {
        int val = atoi(val_str);
        if (val >= 0 && val <= 9) valid = 1;
    }
}

if (!valid) {
    printf("Invalid segment command! (Allowed: start, off, 0-9, show/0-9)\n");
    continue;
}
```

### ② `send_command` 연결 재시도 및 백오프 코드
```c
int retries = 3;
int delay_sec = 1;
while (1) {
    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == 0) {
        break;
    }
    perror("connect");
    close(sockfd);
    sockfd = -1;

    if (--retries <= 0) {
        fprintf(stderr, "Failed to connect to server after multiple attempts.\n");
        return;
    }
    printf("Connection failed. Retrying in %d seconds...\n", delay_sec);
    sleep(delay_sec);
    delay_sec *= 2;

    if((sockfd=socket(AF_INET, SOCK_STREAM, 0))==-1) {
        perror("socket");
        return;
    }
}
```

---

## 4. 검증 결과
- `make client` 빌드가 정상적으로 완료됨.
- 비정상 명령어 입력 시 올바르게 차단됨을 확인.
- Ctrl+C 입력 시 비동기 에러나 데드락 위험 없이 메인 스레드에서 안전하게 클린업 절차를 밟고 종료됨을 확인.
