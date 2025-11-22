#include "myheader.h"

void remove_files(char *path, int all_flag);
void clear_backup_dir(char *path, int *file_count, int *dir_count);

int file_exists(const char *path);
int is_regular_file(const char *path);
int is_directory(const char *path);

int main(int argc, char *argv[])
{
    if (argc < 2) { // exception handling 1
        printf("Usage : %s <FILENAME> [OPTION]\n\t-a : remove all file(recursive)\n\t-c : clear backup directory\n", argv[0]);
        return 1;
    } // 인자 하나만 들어오면 오류 출력

    int opt;
    int flag_a = 0; // a 옵션 플래그 - 디렉토리는 반드시 켜져 있어야 함
    int flag_c = 0; // c 옵션 플래그 - remove -c 이외는 실행 불가능

    if ( (strcmp(argv[0],"remove") ==0 ) && (strcmp(argv[1],"-c") == 0) && (argc < 3)) {
        flag_c = 1; // getopt 오류로 일단 강제로 이렇게 사용...
    }

    while ((opt = getopt(argc -1, &argv[1], "a")) != -1) {
        switch (opt) {
            case 'a':
                flag_a = 1;
                break;
            default:
                printf("Usage : %s <FILENAME> [OPTION]\n\t-a : remove all file(recursive)\n\t-c : clear backup directory\n", argv[0]);
                return 1;
        }
    }
    

    char *home_dir = getenv("HOME"); // 홈디렉토리 먼저 구해서 백업 디렉토리 경로도 얻어 놓았음

    char backup_dir_path[4096];
    snprintf(backup_dir_path, sizeof(backup_dir_path), "%s/%s", home_dir, "backup");
    // backup_dir_path - /home/oslab/backup

    if (flag_c == 1) { // -c 플래그가 1이라는건 argv[0] "remove" argv[1] "-c" 라는 것임
        int file_count = 0;
        int dir_count = 0;
        // 파일과 디렉토리 카운트를 먼저 초기화
        clear_backup_dir(backup_dir_path, &file_count, &dir_count); // 클리어 하면서 위에 값이 카운트 될 것임
        if (file_count == 0 && dir_count == 0) { // exception handling 7 아무 파일도 없었다면 메세지 출력
            printf("No file(s) in the backup\n");
            return 0;
        }
        printf("Backup directory cleared(%d regular files and %d subdirectories totally).\n", file_count, dir_count);
        return 0;
    }

    char *path; // malloc - return...
    if((path =realpath(argv[1], NULL) )== NULL) { // exception handling 1
        printf("Usage : %s <FILENAME> [OPTION]\n\t-a : remove all file(recursive)\n\t-c : clear backup directory\n", argv[0]);
        return 1;
    } // exception handling 6

    if(flag_a == 1 && flag_c == 1){ // exception handling 4
        printf("Usage : %s <FILENAME> [OPTION]\n", argv[0]);
        return 1;
    }

    int len = strlen(path);
    if (len>4096) { // exception handling 2-1
        printf("Error: path length restict is 4,096byte\n");
        free(path);
        return 1;        
    }

    char *src_dirpath = &path[strlen(home_dir)];
    // ex) /P1/a.txt

    char backup_path[4096];
    snprintf(backup_path, 4096, "%s%s", backup_dir_path, src_dirpath);
    // ex) /home/oslab/backup/P1/a.txt
    len = strlen(backup_path);

    if (strncmp(path, backup_dir_path, strlen(backup_dir_path)) == 0) { // exception handling 3
        printf("%s can't be backed up\n", path);
        free(path);
        return 1;
    }

    if (strncmp(path, home_dir, strlen(home_dir)) != 0) { // exception handling 3
        printf("%s can't be backed up\n", path);
        free(path);
        return 1;
    }

    if (!is_regular_file(path) && !is_directory(path)) { // exception handling 2-2
        printf("Error: Not a regular file or directory\n");
        free(path);
        return 1;
    }

    if (access(path, R_OK) != 0) { // exception handling 2-3
        printf("Error: Can't access to %s\n", path);
        free(path);
        return 1;
    }

    if (is_regular_file(path)) { // 레귤러 파일은 파일제거 함수로 삭제 작업
        remove_files(path, flag_a);
    }
    else if (is_directory(path)) { // 디렉토리는 !옵션 a 가 켜져있어야함 !하위에 있는 모든 디렉토리를 재귀탐색하며 파일들을 제거한다
        if (flag_a == 0) { // exception handling 4
            printf("Error : Use -a option to directory\n");
            return 0;
        }
        int file_count = 0;
        int dir_count = 0;
        clear_backup_dir(backup_path, &file_count, &dir_count);
        // printf("Deleted %d files and %d directories.\n", file_count, dir_count);
    }

    free(path);
    return 0;
}


void remove_files(char *path, int all_flag) { // 파일 제거 함수
    char *home = getenv("HOME");
    char backup_path[4096];
    snprintf(backup_path, sizeof(backup_path), "%s/%s", home, "backup");
    // /home/oslab/backup
    char *input_path = &path[strlen(home)];
    // /P1/a.txt

    char backup_path_final[4096];
    snprintf(backup_path_final, 4096, "%s%s", backup_path, input_path);
    // ex) /home/oslab/backup/P1/a.txt
    char *cw_dir = dirname(backup_path_final);
    // ex) /home/oslab/backup/P1

    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    dir = opendir(cw_dir); // 현재 제거하려는 파일이 path인데... 삭제 대상은 backup 디렉토리 하에서 찾겠다는 것임

    int file_count = 0; // 삭제 대상인 파일의 수를 계속해서 증가시킬 변수임
    char check_path[4096];

    int remove_flag = 0; // 하나라도 삭제 한다면 플래그를 켜줄것임 - 옵션 a때문에 설정함

    while ((entry = readdir(dir)) != NULL) {
        snprintf(check_path, sizeof(check_path), "%s/%s", cw_dir, entry->d_name);
        stat(check_path, &file_stat);
        if (S_ISREG(file_stat.st_mode)) {
            // path - /home/oslab/P1/a.txt
            // check_path - /home/oslab/backup/P1/a.txt_백업시간
            char *str1 = basename(path);
            // a.txt
            char *str2 = basename(check_path);
            // a.txt_백업시간
            int length = strlen(str1);
            if ( (strncmp(str1, str2, length) == 0) && all_flag == 0 ) { // a플래그가 꺼져있다면 정보 출력을 할 것임
                if (file_count == 0) printf("0. exit\n");
                setlocale(LC_NUMERIC, ""); // 1,000,000 처리
                printf("%d. %s (%'lld bytes)\n", ++file_count, &check_path[strlen(check_path)-12], file_stat.st_size);// 맥 BSD 는 st_size가 lld로 선언, %'lld로 천자리 숫자 표현
            }
            else if ((strncmp(str1, str2, length) == 0) && all_flag == 1) { // a플래그가 켜져있다 - 전부 다 삭제
                remove(check_path); // 삭제
                printf("\"%s\" backup file removed\n", check_path);
                remove_flag = 1; // 하나라도 삭제 했다면 탈출 위해...
                file_count++; // -a 옵션 쓰고도 파일을 지우지 못했다면 아래에서 메세지 출력하기 위해 카운트한다
            }
        }
    }
    closedir(dir);

    if (remove_flag == 1) return; // 옵션a - 바로 끝

    if (file_count == 0) {
        printf("No backup files found.\n");
        return;
    } // 삭제 대상 파일이 없었다는 것임

    int choice; // 삭제대상을 지정하기 위한 변수
    printf("Choose file to remove\n>> ");
    scanf("%d", &choice); // 사용자에게 입력을 받아서...

    if (choice > 0 && choice <= file_count) { // 0으로 입력하면 아래의 단계를 모두 건너뛴다 - 백업 안하는 것임 | 파일의 수보다 입력을 크게해도 안됨
        dir = opendir(cw_dir);// 다시 한번 읽었던 순서대로 읽을것임
        int index = 0; // 인덱스 0으로 시작
        char check2_path[4096];
        while ((entry = readdir(dir)) != NULL) {
            snprintf(check2_path, sizeof(check2_path), "%s/%s", cw_dir, entry->d_name);
            stat(check2_path, &file_stat);
            if (S_ISREG(file_stat.st_mode)) { // 위와 같은 과정임
                // path - /home/oslab/P1/a.txt check_path - /home/oslab/backup/P1/a.txt
                char *str3 = basename(path);
                char *str4 = basename(check2_path);
                int length = strlen(str3);
                if (strncmp(str3,str4,length) == 0) {
                    index++; // 삭제 대상이 맞다면 인덱스 증가시키고
                    if (index == choice) { // 입력받은 수과 비교해본다 - 같으면 삭제할것임
                        remove(check2_path); // 그냥 삭제
                        printf("\"%s\" backup file removed\n", check2_path);
                        break; // 더이상 탐색할 이유가 없기 때문에 빠져나간다
                    }
                }
            }
        }
        closedir(dir);
    }
}

void clear_backup_dir(char *path, int *file_count, int *dir_count) {
    // 디렉토리 내부를 탐색하며 삭제하는 함수임
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char new_path[PATH_MAX];

    if ((dir = opendir(path)) == NULL) {
        return;
    } // 인자로 받은 디렉토리를 오픈한다 - 무조건 인자로 디렉토리만을 받을 것임

    while ((entry = readdir(dir)) != NULL) { // 안에 들어있는 파일들을 하나씩 읽어볼것임
        snprintf(new_path, sizeof(new_path), "%s/%s", path, entry->d_name);
        // 인자로 받은 (오픈한) 디렉토리의 절대경로에 현재 읽어들인 파일의 이름을 붙였음.
        stat(new_path, &file_stat); // 읽고있는 파일의 정보를 알이 위해 스탯 구조체 가져오기
        if (S_ISDIR(file_stat.st_mode)) { // 만약 읽은 파일이 디렉토리라면
            if ( (strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0) && (strcmp(entry->d_name, ".DS_Store") != 0) ) { // "."현재경로 ".."상위경로 "맥OS파인더 디폴트" 파일은 삭제 대상이 아니다.
                clear_backup_dir(new_path, file_count, dir_count); // 재귀 호출한다 - 현재 읽어들인 디렉토리를
                rmdir(new_path); // 다 내려가서... 하나씩 지우면서 다시 올라감
                (*dir_count)++; // 지웠으면 지웠으니 카운트 증가
            }
        } else if (S_ISREG(file_stat.st_mode)) { // 파일이라면?
            remove(new_path); // 그냥 제거
            printf("\"%s\" backup file removed\n", new_path);
            (*file_count)++; // 카운트 증가
        }
    }
    closedir(dir);
    //주의! 인자로 받은 디렉토리 그 자체는 삭제 대상이 아니다. - 그 아래에 있는 것들이다.
}

int file_exists(const char *path) { // 파일이 존재하는지 확인하는 함수
    if (!path) {
        return 0;
    }
    struct stat st;
    return (stat(path, &st) == 0);
}

int is_regular_file(const char *path) { // 파일이 레귤러인지 확인하는 함수
    if (!path) {
        return 0;
    }
    struct stat st;
    if (stat(path, &st) != 0) {
        printf("file doesn't exist\n");
        return 0;
    }
    return S_ISREG(st.st_mode);
}

int is_directory(const char *path) { // 파일이 디렉토리인지 확인하는 함수
    if (!path) {
        return 0;
    }
    struct stat st;
    if (stat(path, &st) != 0) {
        printf("Directory doesn't exist\n");
        return 0;
    }
    return S_ISDIR(st.st_mode);
}