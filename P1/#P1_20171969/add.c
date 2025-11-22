#include "myheader.h"

int backup_file(char *src_path);
void search_directory(char *src_dir);

int file_exists(const char *path);
int is_regular_file(const char *path);
int is_directory(const char *path);

void create_directory_recursive(const char *dir_path);
void create_directory_path(const char *dir_path);

int get_md5(char *path, unsigned char *result);
int get_sha1(char *path, unsigned char *result);

int main(int argc, char *argv[])
{
    if (argc < 2) { // exception handling 1
        printf("Usage: %s <FILENAME> [OPTION]\n\t-d : add directory recursive\n", argv[0]);
        return 1;
    }

    int opt;
    int flag_dir = 0;

    while ( (opt = getopt(argc-1, &argv[1], "d")) != -1) { // exception handling 4
        switch (opt) {
            case 'd':
                flag_dir = 1;
                break;
            default:
                printf("Usage: %s <FILENAME> [OPTION]\n\t-d : add directory recursive\n", argv[0]);
                return 1;
        }
    }

    char *home_dir = getenv("HOME");
    char *path;

    if((path =realpath(argv[1], NULL) )== NULL) { // exception handling 1
        if (strncmp(argv[1],home_dir,strlen(home_dir)) != 0)
            printf("\"%s\" can't be backuped\n", argv[1]); // exception handling 3
        else
            printf("Usage: %s <FILENAME> [OPTION]\n\t-d : add directory recursive\n", argv[0]);
        return 1;
    }

    int len = strlen(path);
    if (len>4096) { // exception handling 2-1
        printf("Error: path length restict is 4,096byte\n");
        free(path);
        return 1;        
    }
    char backup_dir_path[4096];
    snprintf(backup_dir_path, sizeof(backup_dir_path), "%s/%s", home_dir, "backup");
    // backup_dir_path - $HOME/backup

    if (strncmp(path, backup_dir_path, strlen(backup_dir_path)) == 0) { // exception handling 3
        printf("<%s> can't be backed up\n", path);
        free(path);
        return 1;
    }
    if (strncmp(path, home_dir, strlen(home_dir)) != 0) { // exception handling 3
        printf("<%s> can't be backed up\n", path);
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

    if (is_regular_file(path)) {
        if (flag_dir) { // exception handling 5'
            printf("Error: Don't use -d option to backup regular file\n");
            free(path);
            return 1;
        }
        backup_file(path); // 대상이 파일(정규/레귤러)라면 백업파일 함수 실행
    }
    else if (is_directory(path)) {
        if (!flag_dir) { // exception handling 5
            printf("Error: Use -d option to backup directory\n");
            free(path);
            return 1;
        }        
        search_directory(path); // 대상이 디렉토리라면 재귀적으로 탐색한다 -d 옵션을 사용하지 않으면 에러처리
    }
    free(path);
    return 0;
}

int backup_file(char *src_path) {
    // 인자로 사용자 홈을 포함하는 절대경로를 입력받아 백업디렉토리에 백업한다
    char *home_dir = getenv("HOME");
    // /home/oslab
    char backup_dir_path[PATH_MAX];
    snprintf(backup_dir_path, sizeof(backup_dir_path), "%s/%s", home_dir, "backup");
    // /home/oslab/backup
    char *src_dirpath = &src_path[strlen(home_dir)];
    // src_dirpath - $HOME 제외한 path ex) /P1/a.txt

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now); // 타임스탬프를 위해서 time_t 구조체 불러오기

    char timestamp[14];
    strftime(timestamp, sizeof(timestamp), "_%y%m%d%H%M%S", tm_info); // 백업시간!

    char dst_path[4096];
    snprintf(dst_path, 4096, "%s%s%s", backup_dir_path, src_dirpath, timestamp);
    // /home/oslab/backup + /P1/a.txt + _백업시간

    char *backup_dst = dirname(dst_path);
    // /home/oslab/backup/P1

    // hash test
    char *HASH_FUNC = getenv("HASH_USE");
    if (strcmp(HASH_FUNC, "MD5") == 0) { // MD5는 작동이 잘 안되고 SHA1은 작동이 잘 됨. 이유는...모르겠음
        DIR *dir;
        struct dirent *ent;
        char *dir_path = backup_dst; // 현재 백업하려는 파일이 저장될 디렉토리를 오픈해서 거기있는 파일들을 살펴볼 것임.
        if ((dir = opendir(dir_path)) != NULL) { // 디렉토리를 오픈했다. dir_path는 실제 백업 파일의 절대경로를 dirname() 함수를 통해서 이 파일이 속한 곳의 위치를 구한 것임
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_type == DT_REG) { // 읽어들인 파일이 정규 (레귤러 파일)이라면 비교해본다
                    char path[4096];
                    snprintf(path, 4096, "%s/%s", dir_path, ent->d_name); // 비교할 대상의 절대경로를 구했음
                    unsigned char hash1[4096];
                    unsigned char hash2[4096];
                    get_md5(src_path, hash1); // 인자로 받았던 실제 파일의 해시값이 저장되었음
                    get_md5(path, hash2); // 저장디렉토리에서 읽어들인 파일의 해시값을 저장했음
                    if (memcmp(hash1, hash2, 4096) == 0) { // 비교해서 같다면
                        printf("<%s> is already backuped\n",path); // 저장되어 있던 파일이 이미 백업되어 있었다고 알린 후 함수 리턴
                        closedir(dir);
                        return 1;
                    }
                }
            }
        }
        closedir(dir);
    }
    else if (strcmp(HASH_FUNC, "SHA1") == 0) { // MD5와 정확히 같은 동작을 함
        DIR *dir;
        struct dirent *ent;
        char *dir_path = backup_dst;
        if ((dir = opendir(dir_path)) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_type == DT_REG) {
                    char path[4096];
                    snprintf(path, 4096, "%s/%s", dir_path, ent->d_name);
                    unsigned char hash1[4096];
                    unsigned char hash2[4096];
                    get_sha1(src_path, hash1);
                    get_sha1(path, hash2);
                    if (memcmp(hash1, hash2, 4096) == 0) {
                        printf("<%s> is already backuped\n",path);
                        closedir(dir);
                        return 1;
                    }
                }
            }
        }
        closedir(dir);
    }

    // make directory!
    create_directory_path(backup_dst); // 만약 내가 백업할 파일이 위치할 디렉토리가 존재하지 않는 상태라면 미리 선언한 디렉토리 제작 함수로 만들어줌

    FILE *src_file = fopen(src_path, "rb"); // 입력받은 백업하려는 파일
    FILE *dst_file = fopen(dst_path, "wb"); // 백업 되는 파일 - 백업 시간까지 저장되어 있음

    char buffer[1024];
    int read_size;

    while ((read_size = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        fwrite(buffer, 1, read_size, dst_file);
    }

    fclose(src_file);
    fclose(dst_file);
    // 파일 복사 과정임
    
    printf("%s backuped\n", dst_path); // 복사 끝나면 백업 되었다고 출력

    return 0;
}

void search_directory(char *src_dir) {
    // 디렉토리만을 인자로 받아야함!!
    struct dirent *dir;
    DIR *d = opendir(src_dir); // 인자로 받은 디렉토리를 연다 - 내부에 있는 파일들을 탐색할것임
    while ((dir = readdir(d)) != NULL) { // 내부를 하나씩 읽어본다
        if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0 && strcmp(dir->d_name, ".DS_Store")) { // "."현재경로 ".."상위경로 "맥OS파인더 디폴트" 파일은 백업 대상이 아니다.
            char sub_dir[4096];
            sprintf(sub_dir, "%s/%s", src_dir, dir->d_name);
            // 현재 읽어들인 파일의 절대경로를 만들어주는 과정임
            if (dir->d_type == DT_REG) { // 정규 파일이라면? 백업파일로 백업해준다.
                backup_file(sub_dir);
            }
            else if (dir->d_type == DT_DIR) { // 디렉토리라면? 재귀호출로 다시 탐색한다.
                search_directory(sub_dir);
            }
        }
    } // 결국 디렉토리를 읽어보고... 안에 파일이 없으면 디렉토리는 복사가 되지 않는 구조임. - 파일만 복사가 되고... 파일의 복사를 위해 디렉토리도 만들어지는 것임.
    closedir(d);
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

void create_directory_recursive(const char *dir_path) {
    struct stat st;
    if (stat(dir_path, &st) == -1) {
        // Create the directory if it does not exist
        if (mkdir(dir_path, 0777) != 0) {
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