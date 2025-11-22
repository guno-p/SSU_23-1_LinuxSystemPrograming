#include "myheader.h"

void recover_file(char *filename, char *newfile, int recursive);
void recover_directory(char *dirname, char *newdir);

void create_directory_recursive(const char *dir_path);
void create_directory_path(const char *dir_path);

int get_md5(char *path, unsigned char *result);
int get_sha1(char *path, unsigned char *result);

int main(int argc, char *argv[]) {

    if (argc > 5 || argc < 2) { // exception handling 1
        printf("Usage : %s <FILENAME> [OPTION]\n", argv[0]);
        printf("\t-d : recover directory recursive\n");
        printf("\t-n <NEWNAME> : recover file with new name\n");
        exit(1);
    }
    char *path;
    if((path =realpath(argv[1], NULL) )== NULL) {
        // 파일이 존재하지 않음
    }
    char *home_dir = getenv("HOME");
    // /home/oslab

    char backup_dir_path[4096];
    snprintf(backup_dir_path, sizeof(backup_dir_path), "%s/%s", home_dir, "backup");
    // /home/oslab/backup

    char *src_dirpath = &path[strlen(home_dir)];
    // src_dirpath = /P1/a

    char backup_path[4096];
    snprintf(backup_path, 4096, "%s%s", backup_dir_path, src_dirpath);
    // /home/oslab/backup/P1/a

    char *newfile = NULL; // 널로 시작
    int recursive = 0; // 켜지면 디렉토리여야함!
    
    /* 오류로... 안사용
    int opt;
    while ( (opt = getopt(argc, &argv[0], "dn:")) != -1) { // exception handling 4
        switch (opt) {
            case 'd':
                recursive = 1;
            case 'n':
                newfile = optarg;
            default:
                printf("Usage : %s <FILENAME> [OPTION]\n", argv[0]);
                printf("\t-d : recover directory recursive\n");
                printf("\t-n <NEWNAME> : recover file with new name\n");
                exit(1);
        }
    }*/

    char cwd[4096]; // -n옵션의 new path를 위해 선언

    for(int i=0; i<argc; i++) { // 인자 수동 파싱...
        if(strcmp(argv[i],"-d") == 0) recursive =1; // 디렉토리라면? 켜져있어야함
        if(strcmp(argv[i],"-n") == 0) { // -n 다음에 오는 path를 newfile로 가리킨다
            if(argv[i+1] == NULL) {
                printf("Usage : %s <FILENAME> [OPTION]\n", argv[0]);
                printf("\t-d : recover directory recursive\n");
                printf("\t-n <NEWNAME> : recover file with new name\n");
                exit(1);
            }
            else{
                newfile = argv[i+1];
                if (strlen(newfile)>4096) { // exception handling 2
                    printf("Error : <NEWFILE> length must be under 4096 B\n");
                    return 1;
                }
                getcwd(cwd, sizeof(cwd)); // 현재작업 경로 얻어와서
                snprintf(cwd, sizeof(cwd), "%s/%s", cwd, newfile); // 뒤에다가 입력받은 new path 붙이기

                if (strncmp(cwd, backup_dir_path, strlen(backup_dir_path)) == 0) { // exception handling 3
                    printf("<%s> can't be backed up\n", cwd);
                    free(path);
                    return 1;
                }
                if (strncmp(cwd, home_dir, strlen(home_dir)) != 0) { // exception handling 3
                    printf("<%s> can't be backed up\n", cwd);
                    free(path);
                    return 1;
                }
            }
        }
    }

    if (!recursive) {
        struct stat st;
        stat(path, &st);
        if (S_ISDIR(st.st_mode)) {
            printf("Error : directory must use -d option\n");
            return 1;
        }
    }

    if (recursive && (!newfile)) { // -n 옵션 사용하지 않고 -d 옵션 사용한 경우 : 디렉토리
        recover_directory(backup_path, path);
    }
    else if ( recursive && newfile) { // -n 옵션 사용하고 -d 옵션 사용한 경우 : 디렉토리
        recover_directory(backup_path, cwd);
    }

    if (!newfile) { // 파일이고, -n 옵션 사용하지 않았음
        recover_file(backup_path, path, recursive);
    }
    else if (newfile) { // 파일이고, -n 옵션 사용해서 new path 받았음
        recover_file(backup_path, cwd, recursive);
    }

    return 0;
}

void recover_file(char *filename, char *newfile, int recursive) {
    // 백업디렉토리 기준으로 치환된 경로-백업시간 포함X / 저장할 파일 실제 경로 / -d옵션 확인용
    char *home = getenv("HOME");
    char backup_path[4096];
    snprintf(backup_path, sizeof(backup_path), "%s/%s", home, "backup");
    // $HOME/backup
    char *input_path = &filename[strlen(home)+7]; // /backup 제외하기용 +7
    // 홈디렉토리 아래의 경로 /P1/a.txt

    char backup_path_final[4096];
    snprintf(backup_path_final, 4096, "%s%s", backup_path, input_path);
    // ex) /home/oslab/backup/P1/a.txt
    char *cw_dir = dirname(filename);
    // ex) /home/oslab/backup/P1

    char original_path[4096];
    snprintf(original_path, sizeof(original_path), "%s%s", home, input_path);
    // ex) /home/oslab/P1/a.txt
    // 만든이유는... 해시해보려고 만들었음


    DIR *dir;
    struct dirent *entry;
    dir = opendir(cw_dir); // 현재 백업하려는 파일이 존재하는 디렉토리를 열었음
    struct stat file_stat;
    int file_count = 0;
    while ((entry = readdir(dir)) != NULL) { // 백업 대상이 들어있는 디렉토리를 살펴보면서! 경로가 같은 파일들(백업대상)을 점검
        char check_path[4096];
        snprintf(check_path, sizeof(check_path), "%s/%s", cw_dir, entry->d_name); // 현재 파일의 이름으로 수정되었음 ex) /home/oslab/backup/P1/a_백업시간.txt
        stat(check_path, &file_stat); // 파일인지 디렉토리인지 구분하기 위해 스탯구조체 갖고 옴
        if (S_ISREG(file_stat.st_mode) && (memcmp(entry->d_name,".DS_Store",10) != 0) ) { // 맥OS BSD - 파인더에서 디폴트로 생기는 .DS_Store 거르는 용도
            // filename - /home/oslab/backup/P1/a.txt - recover 수행하고 어느 절대경로로 파일을 만들건지에 대한 정보임 - 인자로 주어졌음
            // check_path - /home/oslab/backup/P1/a.txt_백업시간
            char *str1 = basename(filename); // a.txt
            char *str2 = basename(check_path); // a.txt_백업시간
            int length = strlen(str1); // a.txt 의 길이
            if ( strncmp(str1, str2, length) == 0 ) { // a.txt 길이만큼 비교해서 같다면 백업대상인 파일임
                if (file_count == 0) printf("0. exit\n"); // 만약 첫 출력이라면 파일카운트가 0이기에... 한번만 출력할 것임
                setlocale(LC_NUMERIC, ""); // 1,000,000 처리위한 라이브러리
                printf("%d. %s (%'lld bytes)\n", ++file_count, &check_path[strlen(check_path)-12], file_stat.st_size); // 맥 BSD 는 st_size가 lld로 선언, %'lld로 천자리 숫자 표현
            }
        }
    }
    closedir(dir);

    if (file_count == 0) {
        printf("No file(s) in the backup\n");
        return;
    } // 백업 대상 파일이 백업디렉토리에 없는것임

    int choice;
    printf("Choose file to recover\n>> ");
    scanf("%d", &choice); // 몇번 백업대상을 백업할 것인지 입력받음

    if (choice > 0 && choice <= file_count) { // 0으로 입력하면 아래의 단계를 모두 건너뛴다 - 백업 안하는 것임 | 파일의 수보다 입력을 크게해도 안됨
        dir = opendir(cw_dir); // 다시 한번 읽었던 순서대로 읽을것임
        int index = 0; // 인덱스 0으로 시작
        while ((entry = readdir(dir)) != NULL) {
            char check2_path[4096];
            snprintf(check2_path, sizeof(check2_path), "%s/%s", cw_dir, entry->d_name);
            stat(check2_path, &file_stat);
            if ( S_ISREG(file_stat.st_mode) && (memcmp(entry->d_name,".DS_Store",10) != 0) ) { // 위와 같은 과정임
                // path - /home/oslab/P1/a.txt check2_path - /home/oslab/backup/P1/a.txt
                char *str3 = basename(filename);
                char *str4 = basename(check2_path);
                int length = strlen(str3);
                if (strncmp(str3,str4,length) == 0) {
                    index++; // 백업대상 파일이 맞다면 index를 1 증가시킨다
                    if (index == choice) { // 증가시키고 보니 사용자의 입력 숫자와 같아지면 리커버리 시작
                        char *backup_dst =  dirname(newfile); // 뉴파일은 백업하는 파일이 저장되는 절대경로임
                        // dirname을 통해 포함된 디렉토리까지의 절대경로를 구한다

                        // HASH CHECK!!
                        char *HASH_FUNC = getenv("HASH_USE");
                        if (strcmp(HASH_FUNC, "MD5") == 0) {
                            unsigned char hash1[4096];
                            unsigned char hash2[4096];
                            get_md5(check2_path, hash1); // 백업파일의 해시값이 저장되었음
                            get_md5(original_path, hash2); // 기존 경로에서 읽어들인 파일의 해시값을 저장했음
                            if (memcmp(hash1, hash2, 4096) == 0) { // 비교해서 같다면
                                printf("<%s> is already exist - Check by HASH md5\n",original_path); // 저장되어 있던 파일이 이미 백업되어 있었다고 알린 후 함수 리턴
                                return;
                            }
                        }
                        else if (strcmp(HASH_FUNC, "SHA1") == 0) {
                            unsigned char hash1[4096];
                            unsigned char hash2[4096];
                            get_sha1(check2_path, hash1); // 백업파일의 해시값이 저장되었음
                            get_sha1(original_path, hash2); // 기존 경로에서 읽어들인 파일의 해시값을 저장했음
                            if (memcmp(hash1, hash2, 4096) == 0) { // 비교해서 같다면
                                printf("<%s> is already exist - Check by HASH sha1\n",original_path); // 저장되어 있던 파일이 이미 백업되어 있었다고 알린 후 함수 리턴
                                return;
                            }
                        }

                        create_directory_path(backup_dst);
                        // 선언해둔 재귀적인 디렉토리 생성 함수를 통해 - 파일을 만들어야 하는데 저장해야하는 절대경로까지 "존재하지 않는 디렉토리"는 전부 다 만든다 - 이미 있으면 만들어지지 않음

                        FILE *src = fopen(check2_path, "rb"); // 백업하는 백업디렉토리 하의 사용자로부터 선택된 파일
                        FILE *dst = fopen(newfile, "wb"); // 백업해서 백업되는 절대경로 (-n으로 지정했거나 아니거나...)

                        char buffer[4096];
                        size_t bytes_read;
                        while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                            fwrite(buffer, 1, bytes_read, dst);
                        }

                        fclose(src);
                        fclose(dst);
                        //복사 과정

                        printf("\"%s\" backup recover to ", check2_path);
                        printf("\"%s\"\n", newfile);
                        // 리커버리 끝났으면 정보를 출력
                        break;
                    }
                }
            }
        }
        closedir(dir);
    }
}

void recover_directory(char *dirpath, char *newdir) {
    // 디렉토리는 무조건 이 함수를 통해 재귀탐색 해야한다.
    // 이 함수 내부에서 재귀 탐색 중 발견한 파일은 위에서 설명한 파일 리커버리 함수를 통해 리커버리 한다.
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    dir = opendir(dirpath); // 현재 디렉토리 (인자로 입력받은 절대경로의) 를 연다.
    // 이 내부의 파일들을 검사할 것이고... - 가장 처음에 열었던 디렉토리는 리커버리의 대상이 아니다. - 파일(정규)만이 리커버리 대상이고 디렉토리는 중간 과정에 있는 것임

    int file_count = 0;

    char check_path[4096];
    char new_dirpath[4096];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 && strcmp(entry->d_name, ".DS_Store")) { // "."현재경로 ".."상위경로 "맥OS파인더 디폴트" 파일은 탐색 대상이 아니다.
            snprintf(check_path, sizeof(check_path), "%s/%s", dirpath, entry->d_name); // 입력받은 백업대상 디렉토리에 탐색중인 대상의 이름을 붙여서 절대경로를 완성한다.
            stat(check_path, &file_stat); // 현재 탐색중인 파일이 디렉토리인지 파일인지 구분하기 위해...
            if (S_ISREG(file_stat.st_mode)) { // 파일이면?
                snprintf(new_dirpath, sizeof(new_dirpath), "%s/%s", newdir, entry->d_name); // 입력받은 저장대상 절대경로에 탐색중인 대상의 이름을 붙여서 절대경로를 완성한다.
                char newfile_final_path[4096];
                strncpy(newfile_final_path,new_dirpath,strlen(new_dirpath)-13);
                // path - /home/oslab/P1/a check_path - /home/oslab/backup/P1/a
                recover_file( check_path, newfile_final_path, 0);
                // 백업디렉토리 기준으로 치환된 경로 / 저장할 파일 실제 경로를 넣고... - 방금 위에서 만든 절대경로를 넣고 리커버리 한다.
            }
            else if (S_ISDIR(file_stat.st_mode)) { // 디렉토리면?
                snprintf(new_dirpath, sizeof(new_dirpath), "%s/%s", newdir, entry->d_name);
                recover_directory(check_path, new_dirpath); // 재귀호출을 통해 다시 탐색한다.
            }
            else continue;
        }
    }
    closedir(dir);
}

void create_directory_recursive(const char *dir_path) {
    struct stat st;
    if (stat(dir_path, &st) == -1) { // 파일이 (디렉토리가) 존재하지 않는다면
        if (mkdir(dir_path, 0777) != 0) { // 만든다.
            fprintf(stderr, "Failed to create directory: %s\n", dir_path);
            exit(EXIT_FAILURE);
        }
    }
}

void create_directory_path(const char *dir_path) { // 현재 경로까지의 디렉토리를 만드는 함수
    char *path_copy = strdup(dir_path);
    char *parent_dir_path = dirname(path_copy); // 입력받은 절대경로의 상위 디렉토리를 저장한다

    if (strcmp(parent_dir_path, "/") != 0) { // 최상위 디렉토리가 아니라면...
        create_directory_path(parent_dir_path); // 호출한다
    }
    create_directory_recursive(path_copy); // 본인 절대경로를 인자로 넣는다
    free(path_copy);
}


int get_md5(char *path, unsigned char *result)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return 1;
    }

    MD5_CTX md5_ctx;
    //구조체 선언
    MD5_Init(&md5_ctx);
    //구조체 초기화

    unsigned char buf[1024];
    // 버퍼 선언
    size_t bytes_read;

    while ((bytes_read = fread(buf, 1, 1024, fp)) != 0) {
        MD5_Update(&md5_ctx, buf, bytes_read);
    }
    //md5값 구하기
    MD5_Final(result, &md5_ctx);
    //최종 md5 해시값 result에 저장
    fclose(fp);
    return 1;
}

int get_sha1(char *path, unsigned char *result)
{
    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return 1;
    }

    SHA_CTX sha1_ctx;
    //구조체 선언
    SHA1_Init(&sha1_ctx);
    //구조체 초기화

    unsigned char buf[1024];
    // 버퍼 선언
    size_t bytes_read;
    while ((bytes_read = fread(buf, 1, 1024, fp)) != 0) {
        SHA1_Update(&sha1_ctx, buf, bytes_read);
    }
    //md5값 구하기

    SHA1_Final(result, &sha1_ctx);
    //최종 md5 해시값 result에 저장
    fclose(fp);
    return 1;
}