#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <syslog.h>

#include "ssu_monitor.h"

FILE *log_fp; // log.txt - 변경 추적 대상 디렉터리 안에 생성
char *ID = "20171969";
char *monitor_list = "monitor_list.txt"; // 현재 작업디렉터리에 생성

char *tree_tab = "│   ";
char *tree_mid = "├── ";
char *tree_end = "└── ";

volatile sig_atomic_t signal_received = 0;
// volatile 메모리 위치에 직접 엑세스 / 공유되는 전역변수
// sig_atomic_t 변수 수정은 원자적으로 - 디몬 프로세스 동시 프로그램으로 인해 원자적 처리 필요

void ssu_monitor(int argc, char *argv[]) {
	if (argc > 1) { // 잘못된 실행 에러 처리
		fprintf(stderr, "usage : %s\n", argv[0]);
		exit(1);
	}

	if (signal(SIGUSR1, signal_handler) == SIG_ERR) { // delete 위한 시그널 핸들러 설정
    	fprintf(stderr, "signal() error\n");
    	return;
	}
	ssu_prompt(); // 프롬프트 출력 호출
	return;
}

void ssu_prompt(void) {
	while (1) {
		char command[BUFLEN];
    	char *token;
    	int argc = 0;
		char *argv[16] = {NULL, };

		bzero(command, BUFLEN);

		printf("%s> ", ID);
		fgets(command, BUFLEN, stdin); // 사용자로부터 입력받는 커맨드

		if (*command == '\n') { // 엔터 입력시 프롬프트 재출력
			continue;
		} else {
			command[strlen(command) - 1] = '\0';
		}

		token = strtok(command, " ");
        while (token != NULL) {
            argv[argc] = token;
            argc++;
            token = strtok(NULL, " ");
        } // 토큰화

		if (execute_command(argc, argv) == 1) { // 토큰을 인자로 넘기면서 실행
			break;
		} // 만약 exit명령이라면 1을 리턴받아 프롬프트 출력 반복을 중지, 프로그램 종료
	}
}

int execute_command(int argc, char *argv[]) {
	if (!strcmp(argv[0], "add")) {
		execute_add(argc, argv);
	} else if (!strcmp(argv[0], "delete")) {
		execute_delete(argc, argv);	
	} else if (!strcmp(argv[0], "tree")) {
		execute_tree(argc, argv);
	} else if (!strcmp(argv[0], "help")) {
		execute_help(argc, argv);	
	} else if (!strcmp(argv[0], "exit")) {
		execute_exit(argc, argv);
		return 1;
	} else { // 잘못된 커맨드 입력시 help출력 후 프롬프트 재출력
		fprintf(stderr, "wrong command in prompt\n");
		execute_help(argc, argv);
	}
	return 0;
}

void execute_add(int argc, char *argv[]) {
	char path[BUFLEN];
	char real_path[BUFLEN];
	char line[BUFLEN];
	time_t mn_time;
	struct stat statbuf;
	FILE *fp;
	pid_t pid;

	if (argc != 2 && argc != 4) { // 인자 개수 맞지 않으면 리턴
		help();
		return;
	}

	strcpy(path, argv[1]);

	if (stat(path, &statbuf) < 0) { // path가 존재하지 않는경우 에러
		fprintf(stderr, "in function execute_add: stat error for %s\n", path);
		return;
	}

	if (!S_ISDIR(statbuf.st_mode)) { // 디렉토리가 아닌 경우 에러
		fprintf(stderr, "in function execute_add: %s is not a DIRECTORY\n", path);
		return;
	}

	if (path[0] != '/') // 상대경로면 절대경로로 변환
		realpath(path, real_path);
	else
		strcpy(real_path, path);

	if (argc == 4) { // t옵션 파싱
		if (!strcmp(argv[2], "-t")) {
			char tmp_time[10];
			strcpy(tmp_time, argv[3]);
			mn_time = atoi(tmp_time);
			if (mn_time < 0) { // 입력받은 time이 음수라면 에러
				help();
				return;
			}
		}
		else {// -t로 입력받은게 아니라면 Usage 출력 후 리턴
			help();
			return;
		}
	} else
		mn_time = 1; // -t 옵션이 없는 디폴트는 1초

	// 이미 모니터링 중인 디렉터리는 에러 처리
	if ((fp = fopen(monitor_list, "a+")) == NULL) {
		fprintf(stderr, "in function execute_add: fopen error for %s\n", monitor_list);
		return;
	}
	fseek(fp, 0, SEEK_SET); // "a+"로 열어서 오프셋 맨 뒤를 맨 앞으로 재설정

    // line에 공백단위로 파싱해서 하나씩 확인
    while (fgets(line, sizeof(line), fp)) {
        char* token = strtok(line, " ");
        while (token != NULL) {
            strcpy(path, token);
            if (path[0] == '/') {
                if (check_path(path, real_path)) { // check_path로 새로운 모니터링 대상인 real_path가 이미 모니터링 중인 경로에 포함인지 아닌지 판단
					fprintf(stderr, "in function execute_add: (%s) is already monitoring\n", real_path);
					fclose(fp);
					return;
				}
            }
			token = strtok(NULL, " ");
        }
    }
    fclose(fp);

	init_daemon(real_path, mn_time); // 디몬 프로세스로 만들고 추적 시작

	printf("monitoring started (%s)\n", real_path); // 디몬 프로세스 성공적으로 만들었다면 출력
	return;
}

void execute_delete(int argc, char *argv[]) {
	char path[BUFLEN];
	char real_path[BUFLEN];
	char *tmpfile = "__tmp.txt";
	pid_t pid, dpid;
	FILE *fp, *fp2;
	
	int is_dpid_exist = false;

	// argv[1] 로 받은 pid의 디몬 프로세스 종료

	if (argc != 2) {
        fprintf(stderr, "Usage: delete <DAEMON_PID>\n");
        return;
    }
	if ((pid = atoi(argv[1])) <= 0) {
		fprintf(stderr, "in function execute_delete: <DAEMON_PID> must be positive\n");
		return;
	}
	// daemon의 pid가 monitor_list에 있는지 확인, 없으면 에러 출력
	// 있다면 해당하는 텍스트 라인을 제외하고 복사
	// monitor_list.txt 수정 - dirpath pid 쌍 삭제

	if ((fp = fopen(monitor_list, "r")) == NULL) {
		fprintf(stderr, "in function execute_delete: fopen error for %s\n", monitor_list);
		fclose(fp);
        return;
	}
	if ((fp2 = fopen(tmpfile, "w+")) == NULL) { // 임시 파일
		fprintf(stderr, "in function execute_delete: fopen error for %s\n", tmpfile);
		fclose(fp2);
        return;
	}

	while (fscanf(fp, "%s %d", path, &dpid) != EOF) { // 디렉토리 경로랑 dpid를 읽어옴
		if (dpid != pid) { // 종료 대상 디몬 말고는 전부 복사
            fprintf(fp2, "%s %d\n", path, dpid); // 임시 파일에 복사
		}
		else if (dpid == pid) { // 종료 대상이라면
			is_dpid_exist = true; // 종료 대상 디몬이 있다는 flag를 true로 설정
			*real_path = *path;
		}
	}

	fclose(fp);
	fclose(fp2);
	remove(monitor_list); // monitor_list.txt삭제하고
	rename(tmpfile, monitor_list); // 종료 대상 디몬 말고는 전부 복사가 완료된 임시파일을 monitor_list.txt로 이름 바꾼다

	if(is_dpid_exist) { // 종료 대상 디몬 프로세스가 monitor_list.txt에 있었다면
		if (kill(pid, SIGUSR1) == -1) { // 종료 대상 디몬 프로세스에 signal_received를 1로 설정하는 핸들링 함수가 설정된 시그널 SIGUSR1 을 보낸다
    		fprintf(stderr, "in function execute_delete: kill() error \n");
        	return;
    	}
		printf("monitoring ended (%s)\n", real_path); // 종료 대상 디몬프로세스가 추적하고 있던 디렉토리 모니터링 종료 정보 출력
		// 디몬은 이미 표준출력을 닫은 상태라 delete에서 출력하는 것으로 구현하였음
		return;
	}
	else { // 종료 대상 디몬 프로세스가 monitor_list.txt에 없었다면 에러처리 후 리턴
		fprintf(stderr, "in function execute_delete: <DAEMON_PID> doesn't exist in “monitor_list.txt”\n");
		return;
	}
}

void execute_tree(int argc, char *argv[]) {
	struct stat statbuf;
	char path[BUFLEN];
	char real_path[BUFLEN];
	char line[BUFLEN];
	FILE *fp;
	tree *head;
	int check = 0;

	if (argc != 2) { // 인자 개수 알맞지 않으면 에러 처리
		fprintf(stderr, "Usage : tree <DIRPATH>\n");
		return;
	}

	strcpy(path, argv[1]);

	if (stat(path, &statbuf) < 0) { // 대상이 존재하지 않으면 에러처리
		fprintf(stderr, "in function execute_tree: stat error for %s\n", path);
		return;
	}

	if (!S_ISDIR(statbuf.st_mode)) { // 대상이 디렉토리가 아니라면 에러처리
		fprintf(stderr, "in function execute_tree: %s is not a DIRECTORY\n", path);
		return;
	}

	if (path[0] != '/') // 절대경로로 변환
		realpath(path, real_path);
	else
		strcpy(real_path, path);

	if ((fp = fopen(monitor_list, "r")) == NULL) {
		fprintf(stderr, "in function execute_tree: fopen error for %s\n", monitor_list);
		return;
	}
	fseek(fp, 0, SEEK_SET);

    while (fgets(line, sizeof(line), fp)) {
        char* token = strtok(line, " ");
        while (token != NULL) {
            strcpy(path, token);
            if (path[0] == '/') {
                if (!strcmp(path, real_path)) { // monitor_list.txt 로 관리중인 디몬으로 모니터링 하고 있던 절대경로중에 
				// 입력받은 경로와 같은 것이 있다면 check 를 true로 설정
					check = true;
					break;
				}
            }
			token = strtok(NULL, " ");
        }
    }

	if (!check) { // 대상이 monitor_list.txt에 없었다면
		fprintf(stderr, "in function execute_tree: <DIRPATH> must be in \"monitor_list.txt\"\n");
		fclose(fp);
		return;
	}

	
	fclose(fp);
	head = create_node(real_path, statbuf.st_mode, statbuf.st_mtime);
    make_tree(head, real_path);
    __execute_tree(head, 0); // tree 구조 시각화할 디렉토리를 트리로 만들고 __execute_tree호출
}

void __execute_tree(tree *node, int level) {
	char *filename;

	if (node == NULL)
		return;

	filename = strrchr(node->path, '/');
	filename++; // 경로 다 제외하고 파일 이름만 남겼음

	if (level == 0) {
		printf("%s\n", filename); // 레벨 0이면 파일 이름 출력 - 시각화 대상 디렉토리의 이름이 먼저 출력
		if(node->child)
            __execute_tree(node->child, level+1); // 자식을 level 하나 증가시켜서 재귀호출
        if(node->next)
            __execute_tree(node->next, level); // 형제 노드가 있다면 level은 그대로 두고 출력
		return;
	}
	
	// 레벨이 1이상인 경우에
	for (int i = 0; i < level - 1; i++) {
		printf("%s", tree_tab);
	} // 레벨-1 만큼 tree_tab "│   " 출력

	if (node->next != NULL) { // 노드에 next가 있으면
        printf("%s%s\n", tree_mid, filename);
    } else { // 노드가 마지막이라면
        printf("%s%s\n", tree_end, filename);
    }

	if(node->child) // 자식이 달려있는 디렉토리 노드라서 자식이 있다면
        __execute_tree(node->child, level+1); // 레벨을 하나 증가시켜 재귀적으로 호출
    if(node->next) // 형제 노드가 있다면
        __execute_tree(node->next, level); // 같은 레벨로 재귀적 호출
}

void execute_help(int argc, char *argv[]) {
	help();
}

void execute_exit(int argc, char *argv[]) {
	printf("ssu_monitor exit\n");
	exit(0);
}

void init_daemon(char *dirpath, time_t mn_time) {
	pid_t pid, dpid;
	tree *head, *new_tree;
	char path[BUFLEN];
	FILE *fp;

	sprintf(path, "%s/%s", dirpath, "log.txt");

	head = create_node(dirpath, 0, 0); // 모니터링 시에 변경 기준 old 디렉토리가 되는 head 노드
	make_tree(head, dirpath); // 트리로 먼저 만들어두었음

	if ((pid = fork()) < 0) {
		fprintf(stderr, "in function init_daemon: fork error\n");
		exit(1);
	}
	else if (pid == 0) { // 자식 프로세스에서 디몬 생성, 디렉토리 모니터링
		if ((dpid = (make_daemon())) < 0) { // 디몬 처리, dpid 는 디몬의 pid
			fprintf(stderr, "in function init_daemon: make_daemon failed\n");
			exit(1);
		}
		
		fp = fopen(monitor_list, "a+");
		if(fp == NULL) {
	        fprintf(stderr, "in function init_daemon: fopen error for %s\n", monitor_list);
        	return;
    	}
		fprintf(fp, "%s %d\n", dirpath, dpid); // monitor_list.txt 에 모니터링 대상 디렉토리의 절대경로와 dpid 를 쓰기
		fclose(fp);
		
		while (!signal_received) { // 초기는 0으로 설정 - 반복적으로 모니터링 중
		// 시그널을 받으면 1로 변경, 모니터링 반복 중지
			log_fp = fopen(path, "a+");
			if(log_fp == NULL) {
	        	fprintf(stderr, "in function init_daemon: fopen error for %s\n", path);
        		return;
    		} // log_fp는 전역으로 할당 - compare_tree 에서도 사용 가능

			new_tree = create_node(dirpath, 0, 0);
			make_tree(new_tree, dirpath); // 변경 모니터링을 위해 모니터링 대상 디렉토리를 new_tree로 새로 만들었음

			// 정규파일 변경사항 추적, log.txt에 변경사항 쓰기
			compare_tree(head, new_tree); // 기존 old 와 mn_time이 지나 만든 new 를 비교
			
			free_tree(head);
    		head = new_tree; // 새로 만든 new가 다음 모니터링의 기준이 되는 head가 된다.
			new_tree = NULL;
			
			fclose(log_fp); // 닫아야 log.txt에 쓰기작업 완료.
			sleep(mn_time); // 한번의 모니터링이 끝나면 사용자로부터 -t 옵션으로 입력받은 시간동안 대기 후 다시 모니터링
		}
		// printf("monitoring ended (%s)\n", dirpath);
		exit(0);
	}
	else
		return;
}

pid_t make_daemon(void) {
	pid_t pid;
	int fd, maxfd;

	if ((pid = fork()) < 0) {
		fprintf(stderr, "fork error\n");
		exit(1);
	}
	else if (pid != 0)
		exit(0);
	// (1) 디몬 프로세스는 백그라운드 수행

	setsid();
	// (3) 독자적인 세션을 가져야 한다

	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	// (4) 터미널 입출력 시그널은 무시한다

	maxfd = getdtablesize();
	for (fd = 0; fd < maxfd; fd++) // for debug, fd=3
		close(fd);
	// (7) 모든 파일 디스크립터 닫기
	
	umask(0);
	// (5) 파일 마스킹 금지

	//	chdir("/");
	// 옵션 (6) 현재 디렉토리 루트 디렉토리로 설정

	fd = open("/dev/null", O_RDWR);
	dup(0);
	dup(0);
	// (2) fd 0, 1, 2 를 /dev/null 로 지정 - 라이브러리 루틴 무효화 - 제어 터미널이 없음

	return getpid(); // daemon의 pid
}


tree *create_node(char *path, mode_t mode, time_t mtime) {
	tree *new;

	new = (tree *)malloc(sizeof(tree));
	strcpy(new->path, path);
	new->isEnd = false;
	new->mode = mode;
	new->mtime = mtime;

	new->next = NULL;
	new->prev = NULL;
	new->child = NULL;
	new->parent = NULL;

	return new;
}

void make_tree(tree *dir, char *path) {
	struct dirent **filelist;
	struct stat statbuf;
	char new_path[BUFLEN];
	int count;
	int i;

	if ((count = scandir(path, &filelist, scandir_filter, alphasort)) < 0) { // 이름순 정렬
		fprintf(stderr, "in make_tree: scandir error for %s\n", path);
		return;
	} // scandir_filter로 log.txt 와 monitor_list.txt는 트리 만들기에서 제외

	for (i = 0; i < count; i++) { // 파일 개수만큼 반복
		tree *new_node;
		tree *temp;

		bzero(new_path, BUFLEN);
		sprintf(new_path, "%s/%s", path, filelist[i]->d_name); // 현재 노드 절대경로로 만들어주었음

		if (stat(new_path, &statbuf) < 0) { // 존재 하지 않으면 에러
			fprintf(stderr, "in make_tree: stat error for %s\n", new_path);
			return;
		}

		new_node = create_node(new_path, statbuf.st_mode, statbuf.st_mtime);
		// 노드에는 어떤파일인지 mode와 수정시간 st_mtime 저장

		if (i == 0) { // 첫 파일
			dir->child = new_node; // 부모와 연결
			new_node->parent = dir;
		}
		else { // 첫 파일 제외
			temp = dir->child; // temp는 첫 자식

			while (temp->next != NULL) {
				temp = temp->next; // next 연결이 있다면 쭉 따라와서
			}

			temp->next = new_node; // 마지막 next 즉, null에 새로 할당한 노드를 연결
			new_node->prev = temp;
		}

		if (i == count - 1) // 마지막 파일 끝 표시
			new_node->isEnd = true;

		if (S_ISDIR(statbuf.st_mode)) // 디렉터리라면 재귀적으로 트리 만들기
			make_tree(new_node, new_path);
	}
}

void compare_tree(tree *old, tree *new) {
	time_t curtime;
	struct tm* t;
	char daystr[FILELEN];

	curtime = time(NULL);
	t = localtime(&curtime);

	memset(daystr, 0, FILELEN);
	sprintf(daystr, "%d-%02d-%02d %02d:%02d:%02d", t->tm_year+1900, t->tm_mon+1, 
			t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
	
	if (old == NULL && new == NULL) { // 비교하는 두 노드가 둘 다 NULL
		return;
	} else if (old != NULL && new == NULL) { // old는 있고 new는 NULL
		if (S_ISREG(old->mode)) { // old가 정규파일
			fprintf(log_fp, "[%s][remove][%s]\n", daystr, old->path);
		}
		compare_tree(old->next, NULL);
		compare_tree(old->child, NULL);
	} else if (old == NULL && new != NULL) { // old는 NULL new는 있음
		if (S_ISREG(new->mode)) { // new가 일반파일
			fprintf(log_fp, "[%s][create][%s]\n", daystr, new->path);
		}
		compare_tree(old, new->next);
		compare_tree(old, new->child);
	} else { // 둘 다 존재
		if (!strcmp(old->path, new->path)) { // 이름이 같음 - modify 판별
			if (S_ISREG(old->mode) && S_ISREG(new->mode)) { // 둘 다 일반 파일
				if (old->mtime != new->mtime) {
					time_t modifytime;
					struct tm* tm;
					char mddaystr[FILELEN];
					modifytime = time(NULL);
					tm = localtime(&new->mtime);

					memset(mddaystr, 0, FILELEN);
					sprintf(mddaystr, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon+1, 
							tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
					fprintf(log_fp, "[%s][modify][%s]\n", mddaystr, old->path);
					// 실제 파일 수정시간으로 log 기록
				}
			}
			else if ((old->mode & S_IFMT) != (new->mode & S_IFMT)) {
				// 두 파일 형식이 다른경우 - old는 일반 파일, new는 디렉토리
				// 파일의 형식이 바뀐 경우
				// LINUX 에서는 이름이 같으면 디렉토리, 정규파일이 공존할 수 없음.
			}
			compare_tree(old->next, new->next);
			compare_tree(old->child, new->child);
		} else if (strcmp(old->path, new->path) < 0) { // old가 삭제
			if (S_ISREG(old->mode)) {
				fprintf(log_fp, "[%s][remove][%s]\n", daystr, old->path);
			}
			compare_tree(old->next, new);
			compare_tree(old->next->child, new->child);
		} else { // new가 새로생성
			if (S_ISREG(new->mode)) {
				fprintf(log_fp, "[%s][create][%s]\n", daystr, new->path);
			}
			compare_tree(old, new->next);
			compare_tree(old->child, new->next->child);
		}
	}
}

void free_tree(tree *cur) {
	if (cur->child != NULL)
        free_tree(cur->child);

	if (cur->next != NULL)
        free_tree(cur->next);

    if (cur != NULL) {
		cur->prev = NULL;
		cur->next = NULL;
		cur->parent = NULL;
		cur->child = NULL;
		free(cur);
	}
}

void signal_handler(int signum) { // 전역변수 바꾸는 시그널 핸들러
	signal_received = 1;
}

int scandir_filter(const struct dirent *file) {
	if (!strcmp(file->d_name, ".") || !strcmp(file->d_name, "..")
			|| !strcmp(file->d_name, "log.txt")
			|| !strcmp(file->d_name, monitor_list)) {
		return 0;
	}
	else
		return 1;
}

void help() {
	printf("Usage :\n");
	printf("    > add <DIRPATH> [OPTION]\n");
	printf("        -t <TIME> : monitor every <TIME> seconds\n");
	printf("    > delete <DAEMON_PID>\n");
	printf("    > tree <DIRPATH>\n");
	printf("    > help\n");
	printf("    > exit\n");
}

int check_path(const char* reference_path, const char* path_to_check) {
    if (strcmp(reference_path, path_to_check) == 0)
        return true; // 경로 같은지 확인
    else if (strstr(path_to_check, reference_path) == path_to_check)
        return true; // 포함하는지 확인
    else if (strstr(reference_path, path_to_check) == reference_path)
        return true; // 포함되는지 확인
    else
        return false; // 포함관계 아니면 false 리턴
}