# [Bug] Active Thread Monitor API (/api/threads) 중첩 오류 해결

## 1. 이슈 개요
- **현상**: 웹 대시보드 하단의 "Active Thread Monitor"에 활성 스레드가 표시되지 않으며, 서버 로그에 `Failed to open file: resources/api/threads (error: No such file or directory)` 에러 메시지가 출력되는 현상.
- **원인**: `src/main.c` 파일의 `clnt_connection` 함수에서 `/api/threads` 경로를 핸들링하는 조건 분기(`else if(strcmp(filename, "api/threads") == 0)`)가 `else if(strncmp(filename, "pr/auto/", 8)==0)` 블록 내부에 잘못 중첩(nesting)되어 있어 조건에 도달할 수 없었음.

---

## 2. 세부 문제 상황
- `filename`이 `"api/threads"`일 때, `"pr/auto/"` 문자열로 시작하지 않기 때문에 상위 조건 분기인 `strncmp(filename, "pr/auto/", 8) == 0`을 만족하지 못해 스레드 정보 조회 블록 내부로 들어가지 못함.
- 결국 조건 분기 체인을 통과하여 최하단의 정적 파일 전송 핸들러인 `sendData(clnt_write, type, filename)`로 넘어갔으며, `resources/api/threads` 파일을 여는 시도를 하다가 `ENOENT (No such file or directory)` 오류가 발생했습니다.

---

## 3. 변경 및 해결 내용

### ① 조건문 중첩 해제 및 형제 노드로 분리 (`src/main.c`)
`api/threads` 조건 분기를 `pr/auto/` 조건 분기 블록 바깥으로 이동시켜 독립적인 API 라우팅 노드로 수정했습니다.

```diff
-        } else if(strcmp(filename, "api/threads") == 0) {
-            char response_body[4096] = "[";
-            // ... (스레드 데이터 생성 로직) ...
-            fputs(response_header, clnt_write);
-            fputs(response_body, clnt_write);
-            fflush(clnt_write);
-            goto END;
-        } else {
-            sendError(clnt_write);
-            goto END;
-        }
-    }
+        } else {
+            sendError(clnt_write);
+            goto END;
+        }
+    } else if(strcmp(filename, "api/threads") == 0) {
+        char response_body[4096] = "[";
+        // ... (스레드 데이터 생성 로직) ...
+        fputs(response_header, clnt_write);
+        fputs(response_body, clnt_write);
+        fflush(clnt_write);
+        goto END;
+    }
```

---

## 4. 검증 결과
- **로컬 빌드**: 크로스 컴파일러 환경에서 `make` 명령어로 오류 없이 성공적으로 빌드됨.
- **배포 및 재기동**: `scp`를 통해 Raspberry Pi(`100.66.20.91`)의 `/home/sunbi/prj` 경로에 배포 후 `sudo ./server` 데몬 프로세스로 재기동 완료.
- **API 동작 테스트**:
  - `curl -s http://100.66.20.91:8000/api/threads` 호출 시 정상적으로 active thread 상태 JSON 반환 완료:
    ```json
    [{"tid":548024807776,"ip":"100.72.136.88","cmd":"api/threads","uptime":0}]
    ```
- **웹 대시보드 정상화**: 브라우저 메인 페이지의 "Active Thread Monitor"에 실행 중인 스레드가 정상적으로 표시됨을 확인.
