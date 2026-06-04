# [Fix] 잘못된 장치 명령에 200 OK 반환 및 정적 파일 경로 순회 취약점 해결

## 1. 이슈 개요

### 이슈 A: 잘못된 명령에도 200 OK 반환
- **현상**: `/buzz/abc`, `/led/xyz` 등 잘못된 장치 명령을 보내면 라이브러리 내부에서 `fprintf(stderr, "invalid argument")` 오류를 출력하지만, 서버는 결과와 무관하게 `sendOk()` (HTTP 200 OK)를 반환함.
- **영향**: 클라이언트가 명령 성공 여부를 구분할 수 없음. 평가 과정에서 코드 완성도 1점 감점.

### 이슈 B: 정적 파일 경로 순회 취약점
- **현상**: 정적 파일 요청 시 사용자 입력을 `resources/%s`에 그대로 결합. `../` 경로를 차단하지 않아 서버 파일 시스템의 임의 파일에 접근 가능.
- **부가 문제**: `sendData()`에서 파일을 열기 전에 200 OK 헤더를 먼저 전송하므로, 파일이 존재하지 않아도 클라이언트는 200 OK를 수신함.

---

## 2. 원인 분석

### 이슈 A
장치 드라이버 함수(`led()`, `buzzer()`, `segment()`)의 반환 타입이 `void`로 선언되어 있어, 라이브러리 내부에서 입력 검증을 수행하더라도 그 결과를 서버에 전달할 수 없었음.

서버 라우팅 코드에서는 라이브러리 호출 직후 무조건 `sendOk(clnt_write)`를 호출하는 구조였음:

```c
// 변경 전
hw.buzzer_func(note);
sendOk(clnt_write);   // 항상 200 OK
```

### 이슈 B
`sendData()` 함수에서 `filepath`에 대한 경로 검증이 없었음:

```c
// 변경 전
snprintf(filepath, sizeof(filepath), "resources/%s", filename);
fputs(protocol, fp);  // 200 OK 먼저 전송
fputs(server, fp);
fputs(cnt_type, fp);
fputs(end, fp);
fd = open(filepath, O_RDONLY);  // 파일 열기는 나중에
```

---

## 3. 변경 및 해결 내용

### 이슈 A 해결: 라이브러리 반환 타입 `void` → `int` 변경

**헤더 파일 변경** (`include/led.h`, `include/buzzor.h`, `include/segment.h`):

```diff
-void led(char* arg);
-typedef void (*led_func_t)(char*);
+int led(char* arg);
+typedef int (*led_func_t)(char*);
```

**라이브러리 소스 변경** (`lib/led.c`, `lib/buzzor.c`, `lib/segment.c`):

각 함수에 `int result = 0;` 변수를 추가하고, 잘못된 입력 경로에서 `result = -1`을 설정한 뒤 함수 끝에서 `return result;`로 반환.

```diff
-void led(char *arg)
+int led(char *arg)
 {
+    int result = 0;
     ...
     } else {
         fprintf(stderr, "invalid argument: %s\n", arg);
+        result = -1;
     }
     gpio_unlock();
+    return result;
 }
```

**서버 라우팅 변경** (`src/main.c`):

```diff
-    hw.buzzer_func(note);
-    sendOk(clnt_write);
+    if (hw.buzzer_func(note) < 0) {
+        sendError(clnt_write);
+    } else {
+        sendOk(clnt_write);
+    }
```

### 이슈 B 해결: 경로 순회 차단 및 응답 순서 수정

**`sendData()` 함수 변경** (`src/main.c`):

```diff
+    // 경로 순회 차단
+    if (strstr(filepath, "..") != NULL) {
+        fprintf(stderr, "Path traversal blocked: %s\n", filepath);
+        sendError(fp);
+        return -1;
+    }
+
     fd = open(filepath, O_RDONLY);
     if (fd < 0) {
-        return -1;
+        sendError(fp);
+        return -1;
     }
+
+    // 파일 열기 성공 후 200 OK 전송
+    fputs(protocol, fp);
+    fputs(server, fp);
+    fputs(cnt_type, fp);
+    fputs(end, fp);
```

---

## 4. 변경 파일 목록

| 파일 | 변경 내용 |
|------|-----------|
| `include/led.h` | `void` → `int`, typedef 변경 |
| `include/buzzor.h` | `void` → `int`, typedef 변경 |
| `include/segment.h` | `void` → `int`, typedef 변경 |
| `lib/led.c` | `int` 반환, 실패 시 `-1` |
| `lib/buzzor.c` | `int` 반환, 실패 시 `-1` |
| `lib/segment.c` | `int` 반환, 실패 시 `-1` |
| `src/main.c` | 반환값 기반 `sendOk`/`sendError` 분기, `sendData()` 경로 차단 및 응답 순서 수정 |

---

## 5. 검증 결과

- **빌드**: `make clean && make` — 에러·경고 0개로 빌드 성공
- **기대 동작**:

| 요청 | 변경 전 | 변경 후 |
|------|---------|---------|
| `GET /led/on` | 200 OK | 200 OK |
| `GET /led/abc` | 200 OK | **400 Bad Request** |
| `GET /buzz/do` | 200 OK | 200 OK |
| `GET /buzz/abc` | 200 OK | **400 Bad Request** |
| `GET /segment/start` | 200 OK | 200 OK |
| `GET /segment/abc` | 200 OK | **400 Bad Request** |
| `GET /../../etc/passwd` | 200 OK (빈 본문) | **400 Bad Request** |
| `GET /nonexist.html` | 200 OK (빈 본문) | **400 Bad Request** |
