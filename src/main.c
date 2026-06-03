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
#include <sys/stat.h>
#include <lib/rpi_common.h>
#include <poll.h>
#include <limits.h>

#define MAX_FDS 64

static void* clnt_connection(void* arg);
int sendData(FILE* fp, char* ct, char* filename);
void sendOk(FILE* fp);
void sendError(FILE* fp);
void make_daemon(void);
void* auto_pr_control_thread(void* arg);

struct pollfd fds[MAX_FDS];
int nfds = 1;
volatile int auto_pr_running = 0;
pthread_t auto_pr_thread;
pthread_mutex_t fds_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pr_mutex = PTHREAD_MUTEX_INITIALIZER;

#define PORT 8000

void (*led_func)(char*) = NULL;
void (*buzzer_func)(char*) = NULL;
void (*segment_func)(char*) = NULL;
int (*get_cds_value_func)(void) = NULL;
int (*get_pr_value_func)(void) = NULL;

void *led_handle = NULL;
void *buzzer_handle = NULL;
void *segment_handle = NULL;
void *pr_handle = NULL;

int main(int argc, char *argv[])
{
    int ssock, csock;
    pthread_t threadl;
    struct sockaddr_in serveraddr, cliaddr;
    unsigned int len = sizeof(cliaddr);

    int daemonize = 1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--foreground") == 0) {
            daemonize = 0;
        }
    }

    if (daemonize) {
        make_daemon();
    }

    if(rpi_init() == -1) {
        fprintf(stderr, "rpi_init failed\n");
        return 1;
    }

    // Load dynamic libraries and resolve function symbols
    led_handle = dlopen("./libled.so", RTLD_LAZY);
    if (!led_handle) {
        fprintf(stderr, "Failed to load libled.so: %s\n", dlerror());
    } else {
        led_func = dlsym(led_handle, "led");
    }

    buzzer_handle = dlopen("./libbuzzor.so", RTLD_LAZY);
    if (!buzzer_handle) {
        fprintf(stderr, "Failed to load libbuzzor.so: %s\n", dlerror());
    } else {
        buzzer_func = dlsym(buzzer_handle, "buzzer");
    }

    segment_handle = dlopen("./libsegment.so", RTLD_LAZY);
    if (!segment_handle) {
        fprintf(stderr, "Failed to load libsegment.so: %s\n", dlerror());
    } else {
        segment_func = dlsym(segment_handle, "segment");
    }

    pr_handle = dlopen("./libphotoresistor.so", RTLD_LAZY);
    if (!pr_handle) {
        fprintf(stderr, "Failed to load libphotoresistor.so: %s\n", dlerror());
    } else {
        get_cds_value_func = dlsym(pr_handle, "get_cds_value");
        get_pr_value_func = dlsym(pr_handle, "get_pr_value");
    }

    // 1. 소켓 생성
    if ((ssock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    // 주소 재사용 옵션 설정
    int optval = 1;
    setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // 2. 서버 주소 구조체 설정
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(PORT);

    // 3. 바인드
    if (bind(ssock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("bind");
        close(ssock);
        return 1;
    }

    // 4. 리슨 (최대 대기 큐 10)
    if (listen(ssock, 10) < 0) {
        perror("listen");
        close(ssock);
        return 1;
    }

    printf("Server started on port %d with poll/multi-thread...\n", PORT);

    //poll fd array init
    fds[0].fd = ssock; // listening socket
    fds[0].events = POLLIN;

    for(int i = 1; i < MAX_FDS; i++) {
        fds[i].fd = -1;
    }

    // 5. 클라이언트 접속 수락 루프
    while (1) {
        int ret = poll(fds, nfds, -1);

        if(ret < 0) {
            perror("poll");
            break;
        }

        if(fds[0].revents & POLLIN) {
            csock = accept(ssock, (struct sockaddr *)&cliaddr, &len);
            if (csock >= 0) {
                pthread_mutex_lock(&fds_mutex);
                int added = 0;
                for(int i=0; i<MAX_FDS; i++) {
                    if(fds[i].fd == -1) {
                        fds[i].fd = csock;
                        fds[i].events = POLLIN;
                        if(i >= nfds) {
                            nfds = i+1;
                        }
                        added = 1;
                        break;
                    }
                }
                pthread_mutex_unlock(&fds_mutex);

                if(!added) {
                    fprintf(stderr, "Max clients reached, Closing connection\n");
                    close(csock);
                } else {
                    printf("New Client Connected: fd %d\n", csock);
                }
            }
        }

        for(int i=1; i<nfds; i++) {
            if(fds[i].fd != -1 && (fds[i].revents & POLLIN)) {
                int client_fd = fds[i].fd;

                pthread_mutex_lock(&fds_mutex);
                fds[i].fd = -1;
                if(i==nfds-1) {
                    while(nfds > 1 && fds[nfds - 1].fd == -1) {
                        nfds--;
                    }
                }
                pthread_mutex_unlock(&fds_mutex);
                
                int *arg = malloc(sizeof(*arg));
                *arg = client_fd;

            // 클라이언트 세션을 처리할 스레드 생성
                if (pthread_create(&threadl, NULL, clnt_connection, arg) != 0) {
                    perror("pthread_create");
                    close(client_fd);
                    free(arg);
                } else {
                    pthread_detach(threadl);
                }
            }
        }
    }
    close(ssock);
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

    if (strcmp(filename, "favicon.ico") == 0) {
        char response[] = "HTTP/1.1 204 No Content\r\n\r\n";
        fputs(response, clnt_write);
        fflush(clnt_write);
        goto END;
    }

    if(strncmp(filename, "led/", 4) == 0) {
        char *state = filename + 4;
        if (strcmp(state, "state") == 0) {
            int val = get_led_state();
            char *content = (val == 1) ? "ON" : "OFF";
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

        if (!led_func) {
            fprintf(stderr, "led_func not resolved\n");
            sendError(clnt_write);
            goto END;
        }
        if (strcasecmp(state, "on") == 0) {
            led_func("ON");
        } else if (strcasecmp(state, "mid") == 0) {
            led_func("MID");
        } else if (strcasecmp(state, "off") == 0) {
            led_func("OFF");
        } else if (strncasecmp(state, "brightness?value=", 17) == 0) {
            led_func(state+17);
        } else {
            led_func(state);
        }
        sendOk(clnt_write);
        goto END;
    } else if(strncmp(filename, "buzz/", 5) == 0) {
        char *note = filename + 5;
        if (!buzzer_func) {
            fprintf(stderr, "buzzer_func not resolved\n");
            sendError(clnt_write);
            goto END;
        }
        buzzer_func(note);
        sendOk(clnt_write);
        goto END;
    } else if(strncmp(filename, "segment/", 8) == 0 || strncmp(filename, "ldr/", 4) == 0) {
        char *cmd = (strncmp(filename, "segment/", 8) == 0) ? (filename + 8) : (filename + 4);
        if (!segment_func) {
            fprintf(stderr, "segment_func not resolved\n");
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
        sendOk(clnt_write);
        goto END;
    } else if(strcmp(filename, "pr") == 0) {
        if (!get_cds_value_func) {
            fprintf(stderr, "get_cds_value_func not resolved\n");
            sendError(clnt_write);
            goto END;
        }
        int val = get_cds_value_func();

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
    } else if(strcasecmp(filename, "pr/digital")==0) {
        if (!get_pr_value_func) {
            fprintf(stderr, "get_pr_value_func not resolved\n");
            sendError(clnt_write);
            goto END;
        }
        int val = get_pr_value_func();

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
    } else if(strncmp(filename, "pr/auto/", 8)==0) {
        char *cmd = filename + 8;

        if(strcmp(cmd, "start") == 0) {
            pthread_mutex_lock(&pr_mutex);
            if(!auto_pr_running) {
                auto_pr_running = 1;
                if(pthread_create(&auto_pr_thread, NULL, auto_pr_control_thread, NULL) != 0) {
                    perror("pthread_create auto_pr");
                    auto_pr_running = 0;
                    pthread_mutex_unlock(&pr_mutex);
                    sendError(clnt_write);
                    goto END;
                }
            }
            pthread_mutex_unlock(&pr_mutex);
            sendOk(clnt_write);
            goto END;
        } else if(strcmp(cmd, "stop") == 0) {
            pthread_mutex_lock(&pr_mutex);
            if(auto_pr_running) {
                auto_pr_running = 0;
                pthread_mutex_unlock(&pr_mutex);
                pthread_join(auto_pr_thread, NULL);
            } else {
                pthread_mutex_unlock(&pr_mutex);
            }
            sendOk(clnt_write);
            goto END;
        }
    }

    /* 메시지 헤더를 읽어서 화면에 출력하고 나머지는 무시한다. */
    do {
        fgets(reg_line, BUFSIZ, clnt_read);
        fputs(reg_line, stdout);
        strcpy(reg_buf, reg_line);
        char* buf = strchr(reg_buf, ':');
        (void)buf;
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
    (void)ct;
    /* 클라이언트로 보낼 성공에 대한 응답 메시지 */
    char protocol[ ] = "HTTP/1.1 200 OK\r\n";
    char server[ ] = "Server:Netscape-Enterprise/6.0\r\n";
    char cnt_type[ ] = "Content-Type:text/html\r\n";
    char end[ ] = "\r\n"; 			/* 응답 헤더의 끝은 항상 \r\n */
    char buf[BUFSIZ];
    int fd, len;
    char filepath[BUFSIZ];

    if (strcmp(filename, "") == 0) {
        strcpy(filepath, "resources/index.html");
    } else if (strncmp(filename, "resources/", 10) != 0) {
        snprintf(filepath, sizeof(filepath), "resources/%s", filename);
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
    char server[ ] = "Server: Netscape-Enterprise/6.0\r\n";
    char content_type[ ] = "Content-Type: text/plain\r\n";
    char content_len[ ] = "Content-Length: 2\r\n\r\n";
    char body[ ] = "OK";

    fputs(protocol, fp);
    fputs(server, fp);
    fputs(content_type, fp);
    fputs(content_len, fp);
    fputs(body, fp);
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

// Create Daemon
void make_daemon(void) {
    pid_t pid;
    //first Pid fork
    pid = fork();
    if(pid < 0) exit(EXIT_FAILURE);
    if(pid > 0) exit(EXIT_SUCCESS); // parent process end
    
    //second session create
    if(setsid() < 0) exit(EXIT_FAILURE);

    pid = fork();
    if(pid < 0) exit(EXIT_FAILURE);
    if(pid > 0) exit(EXIT_SUCCESS); // first child process end

    umask(0); // file authority init

    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        char *dir = strrchr(path, '/');
        if (dir != NULL) {
            *dir = '\0'; // truncate executable name to get directory
            if (chdir(path) < 0) {
                perror("chdir to exe dir failed");
                exit(EXIT_FAILURE);
            }
        } else {
            if (chdir("/home/sunbi/projectHome") < 0) {
                if (chdir("/home/sunbi/src") < 0) {
                    perror("chdir failed");
                    exit(EXIT_FAILURE);
                }
            }
        }
    } else {
        if (chdir("/home/sunbi/projectHome") < 0) {
            if (chdir("/home/sunbi/src") < 0) {
                perror("chdir failed");
                exit(EXIT_FAILURE);
            }
        }
    }

    //default file descriptor close
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    //default IO -> /dev/null redirect
    int fd = open("/dev/null", O_RDWR);
    if(fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
    }
}

void* auto_pr_control_thread(void* arg) {
    (void)arg;
    printf("[Auto PR] Thread Started\n");

    while(auto_pr_running) {
        if (get_pr_value_func && led_func) {
            int val = get_pr_value_func();

            if(val == 0) {
                led_func("ON");
            } else {
                led_func("OFF");
            }
        }
        sleep(1);
    }
    printf("[Auto PR] Thread stopped safely\n");
    return NULL;
}