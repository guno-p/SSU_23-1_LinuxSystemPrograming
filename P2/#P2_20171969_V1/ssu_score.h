#ifndef MAIN_H_
#define MAIN_H_

#ifndef true
	#define true 1
#endif

#ifndef false
	#define false 0
#endif

#ifndef STDOUT
	#define STDOUT 1
#endif

#ifndef STDERR
	#define STDERR 2
#endif

#ifndef TEXTFILE
	#define TEXTFILE 3
#endif

#ifndef CFILE
	#define CFILE 4
#endif

#ifndef OVER
	#define OVER 5
#endif
// 프로그램의 실행이 5초 이상의 시간이 걸리는 프로그램은 0점

#ifndef WARNING
	#define WARNING -0.1
#endif
// 프로그램의 실행 결과 채점 시 warning이 발생한 문제는 -0.1점

#ifndef ERROR
	#define ERROR 0
#endif

#define FILELEN 128
#define BUFLEN 1024
#define SNUM 100
#define QNUM 100
#define ARGNUM 5

struct ssu_scoreTable{
	char qname[FILELEN];
	double score;
};

typedef struct IncorrectQuestionNode {
	char question_name[10];
	double score;
	struct IncorrectQuestionNode *next;
} IncorrectQuestionNode;



typedef struct Student {
    int id;
    double score;
    struct Student *next;
} Student; // -s 옵션 위한... 학번, 총점 저장 구조체


void ssu_score(int argc, char *argv[]);
int check_option(int argc, char *argv[]);
void print_usage();

void score_students(char *ansDir); // 정답 디렉토리 안에 score.csv 파일 만들기 위해 인자 넘겨주었음
double score_student(int fd, char *id);
void write_first_row(int fd);

char *get_answer(int fd, char *result);
int score_blank(char *id, char *filename);
double score_program(char *id, char *filename);
double compile_program(char *id, char *filename);
int execute_program(char *id, char *filname);
pid_t inBackground(char *name);
double check_error_warning(char *filename);
int compare_resultfile(char *file1, char *file2);

void do_mOption(char *ansDir); // 수정

int is_thread(char *qname);
void redirection(char *command, int newfd, int oldfd);
int get_file_type(char *filename);
void rmdirs(const char *path);
void to_lower_case(char *c);

void set_scoreTable(char *ansDir);
void read_scoreTable(char *path);
void make_scoreTable(char *ansDir);
void write_scoreTable(char *filename);
void set_idTable(char *stuDir);
int get_create_type();

void sort_idTable(int size);
void sort_idTable_descent(int size); // -s 옵션 추가...

void sort_scoreTable(int size);
void get_qname_number(char *qname, int *num1, int *num2);

IncorrectQuestionNode* reverse_list(IncorrectQuestionNode* head); // 연결리스트 순서 뒤집는 함수 정의
void sort_students(Student **head, int order); // -s 옵션...  정렬하기
void re_score_students(char *ansDir);
double re_score_student(int fd, char *id);

#endif
