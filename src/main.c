#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <wiringPi.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <lib/rpi_common.h>

static void* clnt_connection(void* arg);
int sendData(FILE* fp, char* ct, char* filename);
void sendOk(FILE* fp);
void sendError(FILE* fp);

int main(void)
{
    int ssock;
    pthread_t threadl;
    struct sockaddr_in serveraddr, cliaddr;
    unsigned int len;

    if(rpi_init() == -1) {
        fprintf(stderr, "rpi_init failed\n");
        return 1;
    }

    

    return 0;
}

void *clnt_connection(void *arg)
{
    /* 스레드를 통해서 넘어온 arg를 int 형의 파일 디스크립터로 변환한다. */
    int csock = *((int*)arg);
    FILE *clnt_read, *clnt_write;
    char reg_line[BUFSIZ], reg_buf[BUFSIZ];
    char method[BUFSIZ], type[BUFSIZ];
    char filename[BUFSIZ], *ret;

    /* 파일 디스크립터를 FILE 스트림으로 변환한다. */
    clnt_read = fdopen(csock, "r");
    clnt_write = fdopen(dup(csock), "w");

    /* 한 줄의 문자열을 읽어서 reg_line 변수에 저장한다. */
    fgets(reg_line, BUFSIZ, clnt_read);
    
    /* reg_line 변수에 문자열을 화면에 출력한다. */
    fputs(reg_line, stdout);

    /* ' ' 문자로 reg_line을 구분해서 요청 라인의 내용(메소드)를 분리한다. */
    ret = strtok(reg_line, "/ ");
    strcpy(method, (ret != NULL)?ret:"");
    if(strcmp(method, "POST") == 0) { 		/* POST 메소드일 경우를 처리한다. */
        sendOk(clnt_write); 			/* 단순히 OK 메시지를 클라이언트로 보낸다. */
        goto END;
    } else if(strcmp(method, "GET") != 0) {	/* GET 메소드가 아닐 경우를 처리한다. */
        sendError(clnt_write); 			/* 에러 메시지를 클라이언트로 보낸다. */
        goto END;
    }

    ret = strtok(NULL, " "); 			/* 요청 라인에서 경로(path)를 가져온다. */
    strcpy(filename, (ret != NULL)?ret:"");
    if(filename[0] == '/') { 			/* 경로가 '/'로 시작될 경우 /를 제거한다. */
        for(int i = 0, j = 0; i < BUFSIZ; i++, j++) {
            if(filename[0] == '/') j++;
            filename[i] = filename[j];
            if(filename[j] == '\0') break;
        }
    }

    if(strncmp(filename, "led/", 4) == 0) {
        char *state = filename + 4;
        if (strcmp(state, "state") == 0) {
            int val = digitalRead(1); // LED wiringPi pin 1
            char *content = (val == HIGH) ? "ON" : "OFF";
            char response_buf[BUFSIZ];
            snprintf(response_buf, sizeof(response_buf),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Length: %zu\r\n\r\n"
                     "%s",
                     strlen(content),
                     content);
            fputs(response_buf, clnt_write);
            fflush(clnt_write);
            goto END;
        }

        void *handle = dlopen("./libled.so", RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "dlopen failed: %s\n", dlerror());
            sendError(clnt_write);
            goto END;
        }
        void (*led_func)(char*) = dlsym(handle, "led");
        if (!led_func) {
            fprintf(stderr, "dlsym failed: %s\n", dlerror());
            dlclose(handle);
            sendError(clnt_write);
            goto END;
        }
        if (strcasecmp(state, "on") == 0) {
            led_func("ON");
        } else if (strcasecmp(state, "off") == 0) {
            led_func("OFF");
        } else {
            led_func(state);
        }
        dlclose(handle);
        sendOk(clnt_write);
        goto END;
    } else if(strncmp(filename, "buzz/", 5) == 0) {
        char *note = filename + 5;
        void *handle = dlopen("./libbuzzor.so", RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "dlopen failed: %s\n", dlerror());
            sendError(clnt_write);
            goto END;
        }
        void (*buzzer_func)(char*) = dlsym(handle, "buzzer");
        if (!buzzer_func) {
            fprintf(stderr, "dlsym failed: %s\n", dlerror());
            dlclose(handle);
            sendError(clnt_write);
            goto END;
        }
        buzzer_func(note);
        dlclose(handle);
        sendOk(clnt_write);
        goto END;
    } else if(strncmp(filename, "segment/", 8) == 0 || strncmp(filename, "ldr/", 4) == 0) {
        char *cmd = (strncmp(filename, "segment/", 8) == 0) ? (filename + 8) : (filename + 4);
        void *handle = dlopen("./libsegment.so", RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "dlopen failed: %s\n", dlerror());
            sendError(clnt_write);
            goto END;
        }
        void (*segment_func)(char*) = dlsym(handle, "segment");
        if (!segment_func) {
            fprintf(stderr, "dlsym failed: %s\n", dlerror());
            dlclose(handle);
            sendError(clnt_write);
            goto END;
        }
        if (strcasecmp(cmd, "status") == 0) {
            segment_func("status");
        } else if (strcasecmp(cmd, "off") == 0) {
            segment_func("OFF");
        } else {
            segment_func(cmd);
        }
        dlclose(handle);
        sendOk(clnt_write);
        goto END;
    } else if(strncmp(filename, "pr", 2) == 0) {
        void *handle = dlopen("./libphotoresistor.so", RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "dlopen failed: %s\n", dlerror());
            sendError(clnt_write);
            goto END;
        }
        int (*get_cds_value_func)() = dlsym(handle, "get_cds_value");
        if (!get_cds_value_func) {
            fprintf(stderr, "dlsym failed: %s\n", dlerror());
            dlclose(handle);
            sendError(clnt_write);
            goto END;
        }
        int val = get_cds_value_func();
        dlclose(handle);

        char content[32];
        snprintf(content, sizeof(content), "%d", val);
        char response_buf[BUFSIZ];
        snprintf(response_buf, sizeof(response_buf), 
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/plain\r\n"
                 "Content-Length: %zu\r\n\r\n"
                 "%s", 
                 strlen(content), 
                 content);
        fputs(response_buf, clnt_write);
        fflush(clnt_write);
        goto END;
    }

    /* 메시지 헤더를 읽어서 화면에 출력하고 나머지는 무시한다. */
    do {
        fgets(reg_line, BUFSIZ, clnt_read);
        fputs(reg_line, stdout);
        strcpy(reg_buf, reg_line);
        char* buf = strchr(reg_buf, ':');
    } while(strncmp(reg_line, "\r\n", 2)); 	/* 요청 헤더는 ‘\r\n’으로 끝난다. */

    /* 파일의 이름을 이용해서 클라이언트로 파일의 내용을 보낸다. */
    sendData(clnt_write, type, filename);

END:
    fclose(clnt_read); 				/* 파일의 스트림을 닫는다. */
    fclose(clnt_write);
    pthread_exit(0); 				/* 스레드를 종료시킨다. */

    return (void*)NULL;
}
    
int sendData(FILE* fp, char *ct, char *filename)
{
    /* 클라이언트로 보낼 성공에 대한 응답 메시지 */
    char protocol[ ] = "HTTP/1.1 200 OK\r\n";
    char server[ ] = "Server:Netscape-Enterprise/6.0\r\n";
    char cnt_type[ ] = "Content-Type:text/html\r\n";
    char end[ ] = "\r\n"; 			/* 응답 헤더의 끝은 항상 \r\n */
    char buf[BUFSIZ];
    int fd, len;
    char filepath[BUFSIZ];

    if (strcmp(filename, "") == 0) {
        strcpy(filepath, "src/index.html");
    } else if (strncmp(filename, "src/", 4) != 0) {
        snprintf(filepath, sizeof(filepath), "src/%s", filename);
    } else {
        strcpy(filepath, filename);
    }

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_type, fp);
    fputs(end, fp);

    fd = open(filepath, O_RDONLY); 		/* 파일을 읽기 전용으로 연다. */
    if (fd < 0) {
        fprintf(stderr, "Failed to open file: %s (error: %s)\n", filepath, strerror(errno));
        return -1;
    }
    
    while ((len = read(fd, buf, BUFSIZ - 1)) > 0) {
        buf[len] = '\0';
        fputs(buf, fp);
    }

    close(fd); 					/* 파일을 닫는다. */

    return 0;
}
    
void sendOk(FILE* fp)
{
    /* 클라이언트에 보낼 성공에 대한 HTTP 응답 메시지 */
    char protocol[ ] = "HTTP/1.1 200 OK\r\n";
    char server[ ] = "Server: Netscape-Enterprise/6.0\r\n\r\n";

    fputs(protocol, fp);
    fputs(server, fp);
    fflush(fp);
}
    
void sendError(FILE* fp)
{
    /* 클라이언트로 보낼 실패에 대한 HTTP 응답 메시지 */
    char protocol[ ] = "HTTP/1.1 400 Bad Request\r\n";
    char server[ ] = "Server: Netscape-Enterprise/6.0\r\n";
    char cnt_len[ ] = "Content-Length:1024\r\n";
    char cnt_type[ ] = "Content-Type:text/html\r\n\r\n";

    /* 화면에 표시될 HTML의 내용 */
    char content1[ ] = "<html><head><title>BAD Connection</title></head>";
    char content2[ ] = "<body><font size=+5>Bad Request</font></body></html>";
    printf("send_error\n");

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(cnt_len, fp);
    fputs(cnt_type, fp);
    fputs(content1, fp);
    fputs(content2, fp);
    fflush(fp);
}
