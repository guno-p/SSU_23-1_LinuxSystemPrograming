#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <getopt.h>
#include <limits.h>
#include "ssu_score.h"
#include "blank.h"
// add header files

extern struct ssu_scoreTable score_table[QNUM];
extern char id_table[SNUM][10]; // 학번 저장

struct ssu_scoreTable score_table[QNUM];
char id_table[SNUM][10];

char stuDir[BUFLEN];
char ansDir[BUFLEN];

char newcsvname[BUFLEN]; // -n 옵션 새로운 파일 명
char Args[ARGNUM][10]; // -c 옵션, -p 옵션 출력할 학생 학번 저장
int ArgsCount = 0; // -c, -p 옵션 위한 카운트
int tArgsCount = 0; // -t 옵션 위한 카운트

char errorDir[BUFLEN];
char threadFiles[ARGNUM][FILELEN];

int eOption = false;
int tOption = false;
int mOption = false;
int nOption = false; // -n 옵션 : score.csv를 새로운 이름으로저장
int cOption = false; // -c 옵션 : 입력받은 학생의 점수 출력, 출력도니 학생들의 평균도 출력
int pOption = false; // -p 옵션 : 입력받은 학생의 틀린 문제와 해당 문제의 배점 출력

int sOption = false; // -s 옵션 : <CATEGORY> <1 | -1>
char sCategory[10]; // stdid or score
int is_sOption_inc; // 1 | -1

void ssu_score(int argc, char *argv[])
{
	char saved_path[BUFLEN];
	int i;

	for(i = 0; i < argc; i++){
		if(!strcmp(argv[i], "-h")){ // h옵션
		// argv에 -h 가 있다면 usage 출력 후 종료
			print_usage();
			return;
		}
	}

	memset(saved_path, 0, BUFLEN); // saved_path 초기화
	
	if((argc >= 3) != 0){
		strcpy(stuDir, argv[1]);
		strcpy(ansDir, argv[2]);
	}
	else { // 인자가 2개 이하라면
		print_usage();
		return;
	}

	if(!check_option(argc, argv))
		exit(1);

	getcwd(saved_path, BUFLEN);
	// 현재 작업경로 저장

	if(chdir(stuDir) < 0){ // 입력받은 stuDir 디렉터리로 이동
		fprintf(stderr, "%s doesn't exist\n", stuDir);
		return; // 디렉터리 존재 하지 않으면 에러 출력후 프로그램 종료
	}
	getcwd(stuDir, BUFLEN);
	// stuDir 에 입력받은 디렉터리를 절대경로로 저장

	chdir(saved_path); // 다시 현재 작업경로로 이동

	if(chdir(ansDir) < 0){ // 입력받은 ansDir 디렉터리로 이동
		fprintf(stderr, "%s doesn't exist\n", ansDir);
		return; // 디렉터리 존재 하지 않으면 에러 출력후 프로그램 종료
	}
	getcwd(ansDir, BUFLEN);
	// ansDir 에 입력받은 디렉터리를 절대경로로 저장

	chdir(saved_path); // 다시 현재 작업경로로 이동

	set_scoreTable(ansDir); // ansDir .../ANS_DIR
	set_idTable(stuDir); // stuDir .../STU_DIR

	if(mOption)
		do_mOption(ansDir); // m 옵션 - 점수 배점 수정

	printf("grading student's test papers..\n");

	score_students(ansDir);
	// .../ANS_DIR 디렉토리 안에 score.csv 파일 만들기 위해 인자 넘김
	return;
}

int check_option(int argc, char *argv[])
{
	int i, j;
	int c;

	// getopt_long 사용 위한 옵션 선언
	static struct option long_options[] = {
        {"e", required_argument, NULL, 'e'},
        {"n", required_argument, NULL, 'n'},
        {"c", required_argument, NULL, 'c'},
        {"p", required_argument, NULL, 'p'},
        {"s", required_argument, NULL, 's'},
        {"t", no_argument, NULL, 't'},
        {"m", no_argument, NULL, 'm'},
        {NULL, 0, NULL, 0}
    };

	int option_index = 0;
	int count =0;

	// 옵션 파싱
	while((c = getopt_long(argc, argv, "e:n:c::p::s::tm", long_options, &option_index)) != -1)
	{ // 비옵션 인수 뒤에도 옵션 사용이 가능하도록 getopt_long 으로 교체
	  // -c 옵션 바로 다음에 오는 인수가 옵션이 아님을 명시하기 위해 c:: 로 작성
		switch(c){
			case 'e':
				eOption = true;
				strcpy(errorDir, optarg);
				// errorDir 는 -e 이후 입력한 녀석이 저장

				if(access(errorDir, F_OK) < 0) // 파일 존재?
					mkdir(errorDir, 0755); // 존재하지 않으면 생성
				else{
					rmdirs(errorDir); // 존재한다면 재귀적인 전부 삭제
					mkdir(errorDir, 0755); // 삭제 후에 새로 생성
				}

				char saved_path[BUFLEN];
				memset(saved_path, 0, BUFLEN); // saved_path 초기화
				getcwd(saved_path, BUFLEN); // 현재 작업경로 저장
				if(chdir(errorDir) < 0){ // 입력받은 error 디렉터리로 이동
					fprintf(stderr, "%s doesn't exist\n", errorDir);
					exit(1);
				}
				getcwd(errorDir, BUFLEN); // errorDir 에 error 디렉터리를 절대경로로 저장
				chdir(saved_path); // 다시 현재 작업경로로 이동
				break;

			case 'n': // -n 옵션 : score.csv를 새로운 이름으로저장
				nOption = true;
				strcpy(newcsvname, optarg); // newcsvname에 -n 이후 입력한 파일 이름을 저장
				break;

			case 'c': // -c 옵션 : 입력받은 학생의 점수 출력, 출력도니 학생들의 평균도 출력
				if (count != 0) {
					fprintf(stderr, "Usage: ./ssu_score STUDENT TRUESET -c -p 20190002 20190003\n");
					exit(1);
				} // 가변인자 그룹이 두번 나올 경우 에러 처리
				
				cOption = true;
				while (optind < argc && argv[optind][0] != '-') {
					if (count >= ARGNUM) { // 5개를 넘어가면
						printf("Maximum Number of Argument Exceeded. :: [%s]\n", argv[optind]);
						optind++;
					}
					else {
						strcpy(Args[count], argv[optind]);
						count++;
						optind++;
					}
				}
				ArgsCount = count; // 전역변수 수정
				break;

			case 'p': // -p 옵션 : 입력받은 학생의 점수 출력, 출력도니 학생들의 평균도 출력
				if (count != 0) {
					fprintf(stderr, "Usage: ./ssu_score STUDENT TRUESET -p -c 20190002 20190003\n");
					exit(1);
				} // 가변인자 그룹이 두번 나올 경우 에러 처리
				
				pOption = true;
				while (optind < argc && argv[optind][0] != '-') {
					if (count >= ARGNUM) { // 5개를 넘어가면
						printf("Maximum Number of Argument Exceeded. :: [%s]\n", argv[optind]);
						optind++;
					}
					else { // Args[] 에 학번 저장
						strcpy(Args[count], argv[optind]);
						count++;
						optind++;
					}
				}
				ArgsCount = count; // 전역변수 수정
				break;

			case 's': // s옵션 파싱
				sOption = true;
				strcpy(sCategory, argv[optind++]); // score | stdid
				is_sOption_inc = atoi(argv[optind++]); // 1 | -1 저장

				if(!(strcmp(sCategory, "stdid")) || !(strcmp(sCategory, "score"))) {
					if((is_sOption_inc == 1) || (is_sOption_inc == -1))
						break;
				}
				print_usage();
				exit(1); // stdid score 아니거나 1 -1 아닌경우 에러처리...

			case 't':
				tOption = true;
				i = optind;
				j = 0;

				while(i < argc && argv[i][0] != '-') {

					if(j >= ARGNUM)
						printf("Maximum Number of Argument Exceeded.  :: %s\n", argv[i]);
					else{
						strcpy(threadFiles[j], argv[i]);
					}
					i++; 
					j++;
				}
				tArgsCount = j; // -t 로만 입력하면 0으로 저장
				break;

			case 'm':
				mOption = true;
				break;

			case '?':
				printf("Unkown option %c\n", optopt);
				return false;
		}
	}

	return true;
}

void do_mOption(char *ansDir) // m옵션 - 점수 테이블 파일의 문제 배점 수정하기
{
	double newScore;
	char modiName[FILELEN];
	char filename[FILELEN];
	char *ptr;
	int i;

	ptr = malloc(sizeof(char) * FILELEN);

	while(1){
		printf("Input question's number to modify >> ");
		scanf("%s", modiName); // 수정할 문제 ex) 1-1, 3-1

		if(strcmp(modiName, "no") == 0) // no 입력시 수정 스탑
			break;

		for(i=0; i < sizeof(score_table) / sizeof(score_table[0]); i++){
			strcpy(ptr, score_table[i].qname);
			ptr = strtok(ptr, ".");
			if(!strcmp(ptr, modiName)){ // 수정할 문제
				printf("Current score : %.2f\n", score_table[i].score); // 현재 전역 구조체에 저장된 점수배점
				printf("New score : ");
				scanf("%lf", &newScore); // 입력받을 새로운 배점
				getchar(); // 버퍼 비우기
				score_table[i].score = newScore; // 전역 구조체 배점 수정
				break;
			}
		}
	}

	sprintf(filename, "%s/%s", ansDir, "score_table.csv");
	write_scoreTable(filename); // 수정된 점수배점 전역구조체 score_table 로 스코어 테이블 다시 작성
	free(ptr);
}

void set_scoreTable(char *ansDir)
{
	char filename[FILELEN];

	sprintf(filename, "%s/%s", ansDir, "score_table.csv");
	// filename .../score_table.csv

	// check exist
	if(access(filename, F_OK) == 0)
		read_scoreTable(filename); // 파일 있으면 read_scoreTable 호출
	else{
		make_scoreTable(ansDir); // 없으면 make_scoreTable(ansDir) 호출
		write_scoreTable(filename); // 그리고 write_scoreTable(filename)
	}
}

void read_scoreTable(char *path)
{
	FILE *fp;
	char qname[FILELEN];
	char score[BUFLEN];
	int idx = 0;

	if((fp = fopen(path, "r")) == NULL){
		fprintf(stderr, "file open error for %s\n", path);
		return ;
	} // 인자로 받은 path를 오픈

	while(fscanf(fp, "%[^,],%s\n", qname, score) != EOF){
		strcpy(score_table[idx].qname, qname);
		score_table[idx++].score = atof(score);
	} // 오픈한 파일에서 읽어가며 score_table에 저장

	fclose(fp);
}

void make_scoreTable(char *ansDir)
{// score_table 구조체에 문제당 배점을 저장
	int type, num;
	double score, bscore, pscore;
	struct dirent *dirp; // , *c_dirp;
	DIR *dp; //, *c_dp;
	// char *tmp;
	int idx = 0;
	int i;

	num = get_create_type();
	// 1이랑 2중에 사용자로부터 입력받아 리턴하는 함수

	if(num == 1) // 사용자가 1을 입력 - 모든 문제의 점수를 한번에 부여
	{
		printf("Input value of blank question : ");
		scanf("%lf", &bscore);
		printf("Input value of program question : ");
		scanf("%lf", &pscore);
	}

	if((dp = opendir(ansDir)) == NULL){
		fprintf(stderr, "open dir error for %s\n", ansDir);
		return;
	} // ansDir 로 주어진 디렉터리를 탐색하기 위해 오픈

	while((dirp = readdir(dp)) != NULL){

		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue; // .이랑 ..은 생략

		if((type = get_file_type(dirp->d_name)) < 0)
			continue; // .txt 나 .c 가 아니면 -1이 리턴되는 함수

		strcpy(score_table[idx].qname, dirp->d_name);
		// score_table[]의 qname에 탐색중인 파일의 이름 복사
		idx++;
	}

	closedir(dp);
	sort_scoreTable(idx);
	// scoretable 정렬

	for(i = 0; i < idx; i++) // 점수 입력
	{
		type = get_file_type(score_table[i].qname);
		// .txt .c

		if(num == 1)
		{
			if(type == TEXTFILE)
				score = bscore;
			else if(type == CFILE)
				score = pscore;
		}
		else if(num == 2) // 하나하나 입력받는 2번 입력
		{
			printf("Input of %s: ", score_table[i].qname);
			scanf("%lf", &score);
		}

		score_table[i].score = score;
	}
}

void write_scoreTable(char *filename)
{ // score_table 구조체에 저장되어 있는 문제번호와 점수 셋을 실제로 csv파일로 만드는 함수
	int fd;
	char tmp[BUFLEN];
	int i;
	int num = sizeof(score_table) / sizeof(score_table[0]);
	if((fd = creat(filename, 0666)) < 0){ // 이미 있으면 TRUNC 생성
		fprintf(stderr, "creat error for %s\n", filename);
		return;
	}

	for(i = 0; i < num; i++)
	{
		if(score_table[i].score == 0)
			break;

		sprintf(tmp, "%s,%.2f\n", score_table[i].qname, score_table[i].score);
		write(fd, tmp, strlen(tmp));
	}

	close(fd);
}


void set_idTable(char *stuDir)
{
	struct stat statbuf;
	struct dirent *dirp;
	DIR *dp;
	char tmp[BUFLEN];
	int num = 0;

	if((dp = opendir(stuDir)) == NULL){ // STD_DIR 디렉토리를 열어서 탐색
		fprintf(stderr, "opendir error for %s\n", stuDir);
		exit(1);
	}

	while((dirp = readdir(dp)) != NULL){
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue; // .이랑 .. 은 생략

		sprintf(tmp, "%s/%s", stuDir, dirp->d_name);
		stat(tmp, &statbuf); // 파일의 정보 가지고옴

		if(S_ISDIR(statbuf.st_mode)) // 디렉토리라면 = 학번이라면 id_table에 (char id_table[SNUM][10]) 학번을 저장
			strcpy(id_table[num++], dirp->d_name); // id_table에는 학번이 담겨있음
		else
			continue;
	}
	closedir(dp);

	sort_idTable(num); // id_table 학번 구조체 오름차순 정렬
}

void sort_idTable(int size)
{ // id_table에 저장된 학번을 오름차순 정리
	int i, j;
	char tmp[10];

	for(i = 0; i < size - 1; i++){ // 버블정렬
		for(j = 0; j < size - 1 -i; j++){
			if(strcmp(id_table[j], id_table[j+1]) > 0){
				strcpy(tmp, id_table[j]);
				strcpy(id_table[j], id_table[j+1]);
				strcpy(id_table[j+1], tmp);
			}
		}
	}
}

void sort_idTable_descent(int size)
{ // id_table에 저장된 학번을 내림차순 정리
	int i, j;
	char tmp[10];

	for(i = 0; i < size - 1; i++){ // 버블정렬
		for(j = 0; j < size - 1 -i; j++){
			if(strcmp(id_table[j], id_table[j+1]) < 0){ // 수정
				strcpy(tmp, id_table[j]);
				strcpy(id_table[j], id_table[j+1]);
				strcpy(id_table[j+1], tmp);
			}
		}
	}
} // 내림차순 정리 추가

void sort_scoreTable(int size)
{ // qname - 문제번호 순으로 정렬
	int i, j;
	struct ssu_scoreTable tmp;
	int num1_1, num1_2;
	int num2_1, num2_2;

	for(i = 0; i < size - 1; i++){ // 버블정렬
		for(j = 0; j < size - 1 - i; j++){

			get_qname_number(score_table[j].qname, &num1_1, &num1_2);
			get_qname_number(score_table[j+1].qname, &num2_1, &num2_2);

			if((num1_1 > num2_1) || ((num1_1 == num2_1) && (num1_2 > num2_2))){
				memcpy(&tmp, &score_table[j], sizeof(score_table[0]));
				memcpy(&score_table[j], &score_table[j+1], sizeof(score_table[0]));
				memcpy(&score_table[j+1], &tmp, sizeof(score_table[0]));
			} // 문제번호 순으로 정렬
		}
	}
}

void get_qname_number(char *qname, int *num1, int *num2)
{ // 문제 번호 얻어오기
	char *p;
	char dup[FILELEN];

	strncpy(dup, qname, strlen(qname)); // 복제본
	*num1 = atoi(strtok(dup, "-.")); // -및 . 문제로 분할 토큰화
	// 첫 토큰 정수로 변환하여 저장
	p = strtok(NULL, "-.");
	if(p == NULL)
		*num2 = 0; // 다른 토큰 없으면 0
	else
		*num2 = atoi(p); // 그렇지 않으면 정수로 변환하여 num2에 저장
}

int get_create_type()
{ // 사용자로부터 1이나 2를 입력 받는 함수
	int num;

	while(1)
	{
		printf("score_table.csv file doesn't exist in \"%s\"!\n", ansDir);
		printf("1. input blank question and program question's score. ex) 0.5 1\n");
		printf("2. input all question's score. ex) Input value of 1-1: 0.1\n");
		printf("select type >> ");
		scanf("%d", &num);

		if(num != 1 && num != 2)
			printf("not correct number!\n");
		else
			break;
	}
	return num;
}

void score_students(char *ansDir)
{ //정답 디렉토리 안에 score.csv 파일 만들기 위해 인자 넘겨주었음
	double score = 0; // 평균 내기 위한 누적 합 변수 -c 옵션 켜져있을때만 사용
	int num;
	int fd;
	int i;
	char tmp[BUFLEN];
	int size = sizeof(id_table) / sizeof(id_table[0]);

	char filename[FILELEN];

	if (nOption == true) {// -n 옵션 켜져있다면 입력한 이름으로 파일 저장
	    // newcsvname이 절대경로인지 확인
	    if (newcsvname[0] == '/') { // 절대경로라면
        	strncpy(filename, newcsvname, FILELEN);
    	} else { // 상대경로라면
        	sprintf(filename, "%s/%s", ansDir, newcsvname);
    	}
	} else { // -n 옵션 꺼짐 - ...ANS_DIR/score.csv
	    sprintf(filename, "%s/%s", ansDir, "score.csv");
	}

	if((fd = creat(filename, 0666)) < 0){ // 채점 결과 테이블 생성
		fprintf(stderr, "creat error for %s", filename);
		return;
	}
	write_first_row(fd); // .csv 파일의 첫 줄 입력

	// Student *head = NULL; // -s 옵션

	for(num = 0; num < size; num++)
	{
		bool is_scored = false; // 스코어 입력 처리 되면 true로 변경할 flag
		double tmp_score; // s 옵션

		if(!strcmp(id_table[num], ""))
			break;

		sprintf(tmp, "%s,", id_table[num]);
		write(fd, tmp, strlen(tmp)); // 학번 csv 파일에 입력
		// Student *student = (Student *)malloc(sizeof(Student));
		// student->id = atoi(id_table[num]);

		if(cOption) { // c옵션 켜져있으면
			if (ArgsCount == 0) { // 전체 대상으로 평균구하고, 출력할 것임
				tmp_score = score_student(fd, id_table[num]);
				score += tmp_score; // 학생의 성적을 누적 합 : 평균을 내기 위함
				// student->score = tmp_score; // -s 구조체에 총점 저장
			}
			else { // -c 옵션 뒤에 학번 명시한 경우
				for (i = 0; i < ArgsCount; i++) { // c옵션으로 입력받은 학번 목록을 조회
					if(!strcmp(Args[i], id_table[num])) { // 만약 현재 작업중인 학생이 입력받은 학번 중 한 명인 경우
						tmp_score = score_student(fd, id_table[num]);
						score += tmp_score; // 학생의 성적을 누적 합 : 평균을 내기 위함
						// student->score = tmp_score; // -s 구조체에 총점 저장
						is_scored = true; // 스코어 처리 했으니 플래그 수정
						break; // 반복 종료
					}
				}
				if (!is_scored) {
					tmp_score = score_student(fd, id_table[num]);
					// student->score = tmp_score; // 구조체에 총점 저장
				}
			}
		}
		else {// c옵션 꺼져있는 경우 그냥 입력
			tmp_score = score_student(fd, id_table[num]);
			// student->score = tmp_score; // 구조체에 총점 저장
		}
		// student->next = head;
		// head = student; // 링크드 리스트 연결
	}

	if(cOption) { // c옵션 켜져있으면
		if (ArgsCount == 0)
			printf("Total average : %.2f\n", score / num);
		else
			printf("Total average : %.2f\n", score / ArgsCount);
		// 출력했던 학생들의 성적을 평균내서 출력
		// score (출력한 학생들 성적 누적 합) / ArgsCount (출력한 학생의 수) = 출력 학생 성적 합 평균
	}

	/*
	if(!strcmp(sCategory, "score"))
		sort_students(&head, is_sOption_inc); // 학번, 총점 구조체 점수에 따라 정렬
	*/

	/*
	int idx = 0;
	while (head != NULL) { // 정렬 되었는지 찍어보기..
        *id_table[idx] = head->id;
        head = head->next;
		idx++;
    }
	*/

	
	// s옵션 판단
	// sCategory stdid && -1값이라면 id_table 내림차순 정렬
	if(sOption == true) {
		if(!(strcmp(sCategory, "stdid")) && is_sOption_inc == -1) {
			sort_idTable_descent(size);
			re_score_students(ansDir);
		}
	}
/*
		else if (!(strcmp(sCategory, "score")) && is_sOption_inc == 1) {
			Student *current = head;
			while (current != NULL) {
				for (i = 0; i<size; i++) {
					sprintf(id_table[i], "%d", current->id);
				}
				Student *temp = current;
				current = current->next;
				free(temp);
			}
		} // 점수 오름차순
		else if (!(strcmp(sCategory, "score")) && is_sOption_inc == -1) {
			// head 역순으로 바꾸기
			while (current != NULL) {
				for (i = 0; i<size; i++) {
					sprintf(id_table[i], "%d", current->id);
				}
				Student *temp = current;
				current = current->next;
				free(temp);
			}
		} // 점수 내림차순 정리
*/

//		re_score_students(ansDir);

	printf("result saved.. (%s)\n", filename);
	if(eOption) {
		printf("error saved.. (%s/)\n", errorDir);
	}
	close(fd);
}

void sort_students(Student **head, int order) { // 점수에 따라 정렬
    if (*head == NULL || (*head)->next == NULL) {
        return;
    }// 비어있거나 요소가 하나만 있는경우 정렬하지 않음

    int swapped;
    Student *current;
    Student *prev = NULL;

    do {
        swapped = 0;
        current = *head;

        while (current->next != prev) {
            // 주어진 order값에 따라 점수 비교 (1 ascending | -1 descending)
            if (order * current->score > order * current->next->score) {
                // 총점, 학번 스왑하기
                int temp_id = current->id;
                double temp_score = current->score;
                current->id = current->next->id;
                current->score = current->next->score;
                current->next->id = temp_id;
                current->next->score = temp_score;

                swapped = 1;
            }
            current = current->next;
        }
        prev = current;
    } while (swapped);
}

double score_student(int fd, char *id)
{ // 학생 하나하나의 점수 엑셀파일에 입력하는 함수
	IncorrectQuestionNode *head = NULL;

	int type;
	double result;
	double score = 0; // 개인 점수 총합 초기화 - 누적합 할 변수
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);

	for(i = 0; i < size ; i++)
	{
		if(score_table[i].score == 0)
			break;

		sprintf(tmp, "%s/%s/%s", stuDir, id, score_table[i].qname);
		// .../STD_DIR/20171969/1-1.txt
		// .../STD_DIR/20171969/1-2.txt

		if(access(tmp, F_OK) < 0) // 파일 존재하지 않으면 result는 flase
			result = false;
		else // 파일 존재하면 진행
		{
			if((type = get_file_type(score_table[i].qname)) < 0)
				continue;
			
			if(type == TEXTFILE) {
				result = score_blank(id, score_table[i].qname); // .txt 파일은 빈칸문제 채점 과정
			}
			else if(type == CFILE)
				result = score_program(id, score_table[i].qname); // .c 파일은 프로그램 채점 과정
		}
		if (result == false) { // 오답에 대해 연결 리스트로 문제 이름과 점수를 관리
			IncorrectQuestionNode *new_node = (IncorrectQuestionNode *)malloc(sizeof(IncorrectQuestionNode));
			char tmp[10]; // 임시
			strcpy(tmp, score_table[i].qname); // 복사
			char *ptr = strchr(tmp, '.'); // .가리켜서
			if (ptr != NULL) *ptr = '\0'; // 널로 바꾸기
			strncpy(new_node->question_name, tmp, 10); // 문제번호 저장
			new_node->score = score_table[i].score; // 점수 저장
			new_node->next = head;
			head = new_node;
			write(fd, "0,", 2); // csv 파일에 0점으로 적용
		} else { // 오답이 아닌 경우에
			if(result == true){ // score_blank()와 score_program() 의 결과값 result가 true
				score += score_table[i].score; // 현재 학생 총점 수정
				sprintf(tmp, "%.2f,", score_table[i].score); // 해당 점수 배점
			}
			else if(result < 0){ // 감점
				score = score + score_table[i].score + result; // 감점 result 반영
				sprintf(tmp, "%.2f,", score_table[i].score + result); // 해당 점수 배점에 감점
			}
			write(fd, tmp, strlen(tmp)); // .csv 파일에 "점수," 쓰기
		}
	}

	if(cOption) { // c옵션 켜져있으면
		if(ArgsCount == 0) {
			printf("%s is finished. score : %.2f", id, score); // 점수 합 출력
			// 20171969 is finished. score : 점수 합
		}
		else{
			bool is_scored = false; // 스코어 입력 처리 되면 true로 변경할 flag
			for (i = 0; i < ArgsCount; i++) { // c옵션으로 입력받은 학번 목록을 조회
				if(!strcmp(Args[i],id)){ // 만약 현재 작업중인 학생이 입력받은 학번 중 한 명인 경우
					printf("%s is finished. score : %.2f", id, score); // 점수 합 출력
					// 20171969 is finished. score : 점수 합
					is_scored = true;
					break;
				}
			}
			if (!is_scored) // -c 이후 작성한 학생이 아닌 경우
				printf("%s is finished.", id); // 수행 완료 출력
		}
	}
	else // c 옵션 꺼져있으면
		printf("%s is finished.", id); // 수행 완료 출력

	if(pOption) { // p옵션 켜져있으면
		IncorrectQuestionNode *current = reverse_list(head);
		if (ArgsCount == 0) { // 학번 지정 x -> 모든 학생의 오답 출력
			if(cOption) printf(",");
			printf(" wrong problem : ");
			while (current != NULL) {
				printf("%s(%.2f) ", current->question_name, current->score);
				IncorrectQuestionNode *temp = current;
				current = current->next;
				free(temp);
			}
		}
		else { // 학번 지정 -> 지정된 학생만 출력
			for (i = 0; i < ArgsCount; i++) { // c옵션으로 입력받은 학번 목록을 조회
				if(!strcmp(Args[i],id)){ // 만약 현재 작업중인 학생이 입력받은 학번 중 한 명인 경우
					if(cOption) printf(",");
					printf(" wrong problem : ");
					while (current != NULL) {
						printf("%s(%.2f) ", current->question_name, current->score);
						IncorrectQuestionNode *temp = current;
						current = current->next;
						free(temp);
					}
					break;
				}
			}
		}
	}
	printf("\n");

	sprintf(tmp, "%.2f\n", score);
	write(fd, tmp, strlen(tmp)); // sum에는 점수 합을 쓰기

	return score;
}

IncorrectQuestionNode* reverse_list(IncorrectQuestionNode* head) {
    IncorrectQuestionNode* prev = NULL;
    IncorrectQuestionNode* current = head;
    IncorrectQuestionNode* next = NULL;
    while (current != NULL) {
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    return prev;
} // 연결리스트 역순 만들기 함수 정의

void write_first_row(int fd)
{
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);

	write(fd, ",", 1); // 1,B부터 쓰기 위해

	for(i = 0; i < size; i++){
		if(score_table[i].score == 0)
			break; // 다 쓰면 끝
		
		sprintf(tmp, "%s,", score_table[i].qname); // score_table의 qname 에는 1-1.txt 1-2.txt ... 이 저장되어 있음
		write(fd, tmp, strlen(tmp)); // 1-1.txt,1-2.txt,1-3.txt,2-1.txt, ...
	}
	write(fd, "sum\n", 4); // 마지막에 sum 열을 추가하고 개행
	// 다음 행부터는 학생들의 학번과... 채점 점수가 입력 될 것
}

char *get_answer(int fd, char *result)
{
	char c;
	int idx = 0;

	memset(result, 0, BUFLEN);
	while(read(fd, &c, 1) > 0)
	{ // 한 문자씩 읽어보면서
		if(c == ':') // :를 만나면
			break;
		
		result[idx++] = c; // result에는 그동안 읽은 문자 저장
	}

	if(result[strlen(result) - 1] == '\n') // 마지막 개행문자
		result[strlen(result) - 1] = '\0'; // 널로 바꾸기

	return result;
}

void re_score_students(char *ansDir)
{ //정답 디렉토리 안에 score.csv 파일 만들기 위해 인자 넘겨주었음
	int num;
	int fd;
	char tmp[BUFLEN];
	int size = sizeof(id_table) / sizeof(id_table[0]);

	char filename[FILELEN];

	if (nOption == true) {// -n 옵션 켜져있다면 입력한 이름으로 파일 저장
	    // newcsvname이 절대경로인지 확인
	    if (newcsvname[0] == '/') { // 절대경로라면
        	strncpy(filename, newcsvname, FILELEN);
    	} else { // 상대경로라면
        	sprintf(filename, "%s/%s", ansDir, newcsvname);
    	}
	} else { // -n 옵션 꺼짐 - ...ANS_DIR/score.csv
	    sprintf(filename, "%s/%s", ansDir, "score.csv");
	}

	if((fd = creat(filename, 0666)) < 0){ // 채점 결과 테이블 생성 - TRUNC
		fprintf(stderr, "creat error for %s", filename);
		return;
	}
	write_first_row(fd); // .csv 파일의 첫 줄 입력

	for(num = 0; num < size; num++)
	{
		if(!strcmp(id_table[num], ""))
			break;

		sprintf(tmp, "%s,", id_table[num]);
		write(fd, tmp, strlen(tmp)); // 학번 csv 파일에 입력

		re_score_student(fd, id_table[num]); // 학생의 성적 하나하나 입력...
	}
	close(fd);
}

double re_score_student(int fd, char *id)
{ // 학생 하나하나의 점수 엑셀파일에 입력하는 함수 -s 옵션용
	int type;
	double result;
	double score = 0; // 개인 점수 총합 초기화 - 누적합 할 변수
	int i;
	char tmp[BUFLEN];
	int size = sizeof(score_table) / sizeof(score_table[0]);

	for(i = 0; i < size ; i++)
	{
		if(score_table[i].score == 0) // 0점문제 스킵
			break;

		sprintf(tmp, "%s/%s/%s", stuDir, id, score_table[i].qname);
		// .../STD_DIR/20171969/1-1.txt

		if(access(tmp, F_OK) < 0) // 파일 존재하지 않으면 result는 flase
			result = false;
		
		else // 파일 존재하면 진행
		{
			if((type = get_file_type(score_table[i].qname)) < 0)
				continue;
			
			if(type == TEXTFILE) {
				result = score_blank(id, score_table[i].qname); // .txt 파일은 빈칸문제 채점 과정
			}
			else if(type == CFILE)
				result = score_program(id, score_table[i].qname); // .c 파일은 프로그램 채점 과정
		}
		if (result == false) { // 오답에 대해 연결 리스트로 문제 이름과 점수를 관리
			write(fd, "0,", 2); // csv 파일에 0점으로 적용
		} else { // 오답이 아닌 경우에
			if(result == true){ // score_blank()와 score_program() 의 결과값 result가 true
				score += score_table[i].score; // 현재 학생 총점 수정
				sprintf(tmp, "%.2f,", score_table[i].score); // 해당 점수 배점
			}
			else if(result < 0){ // 감점
				score = score + score_table[i].score + result; // 감점 result 반영
				sprintf(tmp, "%.2f,", score_table[i].score + result); // 해당 점수 배점에 감점
			}
			write(fd, tmp, strlen(tmp)); // .csv 파일에 "점수," 쓰기
		}
	}
	sprintf(tmp, "%.2f\n", score);
	write(fd, tmp, strlen(tmp)); // sum에는 점수 합을 쓰기

	return score;
}

int score_blank(char *id, char *filename) // 학생의 답변과 정답을 비교하여 빈칸 채우기 문제에 점수를 채점
{ // 인자는 학번, 1-1.txt
	char tokens[TOKEN_CNT][MINLEN];
	node *std_root = NULL, *ans_root = NULL;
	int idx;//, start;
	char tmp[BUFLEN];
	char s_answer[BUFLEN], a_answer[BUFLEN];
	// 학생 답, 정답
	char qname[FILELEN];
	int fd_std, fd_ans;
	int result = true;
	int has_semicolon = false;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));

	sprintf(tmp, "%s/%s/%s", stuDir, id, filename);
	// ...STD_DIR/20171969/1-1.txt
	fd_std = open(tmp, O_RDONLY);
	// fd_std는 채점 대상 학생의 파일
	
	// strcpy(s_answer, get_answer(fd_std, s_answer));
	get_answer(fd_std, s_answer);
	// s_answer 설정 - 학생 답

	if(!strcmp(s_answer, "")){ // 답 입력이 없으면 false 리턴 - 오답
		close(fd_std);
		return false;
	}

	if(!check_brackets(s_answer)){ // blank.c
	// 괄호의 개수가 맞지 않으면 0을 리턴
		close(fd_std);
		return false; // 괄호의 개수가 맞지 않으면 false 리턴
	}

	strcpy(s_answer, ltrim(rtrim(s_answer)));
	// s_answer의 좌 우 공백을 제거

	if(s_answer[strlen(s_answer) - 1] == ';'){
		has_semicolon = true;
		s_answer[strlen(s_answer) - 1] = '\0';
	} // 세미콜론을 널로 바꿈

	if(!make_tokens(s_answer, tokens)){ // s_answer 을 toekns 로 token화 한다
		close(fd_std);
		return false; // 오답
	}

	idx = 0;
	std_root = make_tree(std_root, tokens, &idx, 0); // 토큰화한 학생답을 std_root로 트리화 한다 - 정답 트리도 같은 과정으로 트리화하고 같은지 비교할것임

	sprintf(tmp, "%s/%s", ansDir, filename);
	// ...ANS_DIR/1-1.txt
	fd_ans = open(tmp, O_RDONLY);

	while(1)
	{
		ans_root = NULL;
		result = true;

		for(idx = 0; idx < TOKEN_CNT; idx++)
			memset(tokens[idx], 0, sizeof(tokens[idx]));

		// strcpy(a_answer, get_answer(fd_ans, a_answer));
		get_answer(fd_ans, a_answer);
		// 답안 가져오기

		if(!strcmp(a_answer, "")) // 답안 공백은 break!
			break;

		strcpy(a_answer, ltrim(rtrim(a_answer)));
		// 답안도 좌우 공백 삭제 등 학생 답과 같은 과정

		if(has_semicolon == false){ // 학생 답에 세미콜론이 없었으면
			if(a_answer[strlen(a_answer) -1] == ';') // 근데 답안에는 세미콜론이 있으면
				continue;
		}

		else if(has_semicolon == true) // 학생 답에 세미콜론이 있으면
		{
			if(a_answer[strlen(a_answer) - 1] != ';')
				continue; // 근데 답안에 세미콜론이 없으면
			else
				a_answer[strlen(a_answer) - 1] = '\0'; // 답안에 세미 콜론이 있으면 널로바꾸기
		}

		if(!make_tokens(a_answer, tokens)) // 답안을 토큰화
			continue;

		idx = 0;
		ans_root = make_tree(ans_root, tokens, &idx, 0); // 답안 토큰을 트리화

		compare_tree(std_root, ans_root, &result); // 학생답과 정답 트리를 비교해서 result에!!!

		if(result == true){ // 같음
			close(fd_std);
			close(fd_ans);

			if(std_root != NULL)
				free_node(std_root);
			if(ans_root != NULL)
				free_node(ans_root);
			return true;
		}
	}
	
	close(fd_std);
	close(fd_ans);

	if(std_root != NULL)
		free_node(std_root);
	if(ans_root != NULL)
		free_node(ans_root);

	return false; // 오답
}

double score_program(char *id, char *filename)
{ // .c 프로그램을 컴파일한 다음 프로그램을 실행, 확인하여 C 프로그래밍 문제를 채점
	double compile;
	int result;

	compile = compile_program(id, filename);

	if(compile == ERROR || compile == false)
		return false;
	
	result = execute_program(id, filename);

	if(!result) // execut_program의 결과가 false 즉 프로그램 실행 결과가 답안 프로그램 실행 결과가 다름
		return false; // 오답

	if(compile < 0)
		return compile; // 컴파일의 결과 - 감점

	return true;
}

int is_thread(char *qname)
{
	int i;
	int size = sizeof(threadFiles) / sizeof(threadFiles[0]); // -t 옵션에서 받았던 threadFiles
	
	if (tArgsCount == 0)
		return true;

	for(i = 0; i < size; i++){
		if(!strcmp(threadFiles[i], qname)) // -t 옵션 인수로 준 문제 번호와 qname 이 같다면
			return true;
	}
	return false;
}

double compile_program(char *id, char *filename)
{ // 학생과 정답의 C 프로그램을 모두 컴파일하고 오류나 경고가 있는지 확인
	int fd;
	char tmp_f[BUFLEN], tmp_e[BUFLEN];
	char command[BUFLEN];
	char qname[FILELEN];
	int isthread;
	off_t size;
	double result;
	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));
	
	isthread = is_thread(qname);

	sprintf(tmp_f, "%s/%s", ansDir, filename);
	sprintf(tmp_e, "%s/%s.exe", ansDir, qname);

	if(tOption && isthread) // -t옵션이면 -lpthread 옵션 추가
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f); // MacOS POSIX 스레드를 사용 - 컴파일할 때 -lpthread 대신 -pthread 옵션을 사용		
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);
	
	// printf("%s\n", command);
	// -lpthread 옵션 command 적용되는지 확인

	sprintf(tmp_e, "%s/%s_error.txt", ansDir, qname);
	fd = creat(tmp_e, 0666);

	redirection(command, fd, STDERR); // STDERR 로 출력되는 것이 fd에 쓰여지도록 리디렉션.
	size = lseek(fd, 0, SEEK_END); // filesize
	close(fd);
	unlink(tmp_e);

	if(size > 0) // 정답파일은 error 발생하는가... 실행 환경에 따라 발생할 수도...
		return false;

	sprintf(tmp_f, "%s/%s/%s", stuDir, id, filename);
	sprintf(tmp_e, "%s/%s/%s.stdexe", stuDir, id, qname);

	if(tOption && isthread) // -t 옵션이면 -lpthread 옵션 추가
		sprintf(command, "gcc -o %s %s -lpthread", tmp_e, tmp_f); // MacOS POSIX 스레드를 사용 - 컴파일할 때 -lpthread 대신 -pthread 옵션을 사용할수도..
	else
		sprintf(command, "gcc -o %s %s", tmp_e, tmp_f);

	// printf("%s\n", command);
	// -lpthread 옵션 command 적용되는지 확인

	sprintf(tmp_f, "%s/%s/%s_error.txt", stuDir, id, qname);
	fd = creat(tmp_f, 0666);

	redirection(command, fd, STDERR);
	size = lseek(fd, 0, SEEK_END);
	close(fd);

	if(size > 0){ // 에러가 있어서 에러 임시 파일이 생겼다면
		if(eOption) // -e옵션 : 에러가 난 문제에 대해 에러메세지를 txt파일로 저장
		{
			sprintf(tmp_e, "%s/%s", errorDir, id);
			// .../error(사용자가 입력)/20171969
			if(access(tmp_e, F_OK) < 0) // 에러 저장 디렉토리 없는경우
				mkdir(tmp_e, 0755); // 생성

			sprintf(tmp_e, "%s/%s/%s_error.txt", errorDir, id, qname);
			// .../error(사용자가 입력)/20171969/29_error.txt
			rename(tmp_f, tmp_e); // old->new 즉, tmp_f -> tmp_e

			result = check_error_warning(tmp_e); // result는 에러는 0 warning은 발생 수에 따라 감점
		}
		else{ 
			result = check_error_warning(tmp_f);
			unlink(tmp_f);
		}

		return result;
	}
	unlink(tmp_f);
	return true;
}

double check_error_warning(char *filename)
{ // 오류 및 경고에 대한 컴파일 프로세스의 출력을 확인
  // 오류 또는 경고의 존재에 따라 리턴...
	FILE *fp;
	char tmp[BUFLEN];
	double warning = 0;

	if((fp = fopen(filename, "r")) == NULL){
		fprintf(stderr, "fopen error for %s\n", filename);
		return false;
	}

	while(fscanf(fp, "%s", tmp) > 0){
		if(!strcmp(tmp, "error:")) // 만약 error: 가 있다 - 에러가 발생했다
			return ERROR; // 0점 처리 > ssu_score.h define에서 수정 가능
		else if(!strcmp(tmp, "warning:"))
			warning += WARNING; // warning 발생시 define 한 WARNING 감점(0.1)에 따라 warning 에 누적 합
	}
	return warning; // warning으로 감점된 총 점수
}

int execute_program(char *id, char *filename)
{ //  학생의 컴파일된 프로그램을 실행하고 출력을 정답의 출력과 비교
	char std_fname[BUFLEN], ans_fname[BUFLEN];
	char tmp[BUFLEN];
	char qname[FILELEN];
	time_t start, end;
	pid_t pid;
	int fd;

	memset(qname, 0, sizeof(qname));
	memcpy(qname, filename, strlen(filename) - strlen(strrchr(filename, '.')));
	// qname : 문제번호

	sprintf(ans_fname, "%s/%s.stdout", ansDir, qname); // 답안출력 결과 저장될 파일을 .../ANS_DIR/20.stdout으로 만들었음
	fd = creat(ans_fname, 0666);

	sprintf(tmp, "%s/%s.exe", ansDir, qname);
	redirection(tmp, fd, STDOUT); // .../ANS_DIR/20.exe 파일의 실행 결과인 STDOUT 표준출력이 fd로 리디렉션되어 저장됨
	close(fd);

	sprintf(std_fname, "%s/%s/%s.stdout", stuDir, id, qname); // 학생출력 결과 저장될 파일을 .../STU_DIR/20.stdout으로 만들었음
	fd = creat(std_fname, 0666);

	sprintf(tmp, "%s/%s/%s.stdexe &", stuDir, id, qname); // 학생 프로그램은 백그라운드 실행

	start = time(NULL); // 시간이 초과되면 0점처리 하기 위해 시간측정 시작
	redirection(tmp, fd, STDOUT); // .../STU_DIR/20.stdexe 파일의 백그라운드 실행 결과인 STDOUT 표준출력이 fd(.../STU_DIR/20.stdout)로 리디렉션되어 저장됨
	
	sprintf(tmp, "%s.stdexe", qname);
	while((pid = inBackground(tmp)) > 0){
		end = time(NULL);
		// end 와 start 타임의 차이가 5초보다 크다면...
		if(difftime(end, start) > OVER){ // 5초 이상으로 걸렸다면
			kill(pid, SIGKILL); // 프로세스 킬
			close(fd);
			return false; // 오답처리
		}
	}
	close(fd);

	return compare_resultfile(std_fname, ans_fname); // 두 파일 비교, 같다면 true
}

pid_t inBackground(char *name)
{
	pid_t pid;
	char command[64];
	char tmp[64];
	int fd;
	// off_t size;
	
	memset(tmp, 0, sizeof(tmp));
	fd = open("background.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
	// background.txt 파일 생성 - O_TRUNC 덮어쓰기

	sprintf(command, "ps | grep %s | grep -v grep", name); // 20171969 mac 환경 command 수정
	redirection(command, fd, STDOUT);

	lseek(fd, 0, SEEK_SET); // 오프셋 0으로
	read(fd, tmp, sizeof(tmp));

	if(!strcmp(tmp, "")){ // 결과가 공백
		unlink("background.txt");
		close(fd);
		return 0;
	}

	pid = atoi(strtok(tmp, " "));
	close(fd);

	unlink("background.txt");
	return pid;
}

int compare_resultfile(char *file1, char *file2)
{ // 학생의 프로그램과 정답의 프로그램의 출력 파일을 비교하여 동일한지 판단
	int fd1, fd2;
	char c1, c2;
	int len1, len2;

	fd1 = open(file1, O_RDONLY);
	fd2 = open(file2, O_RDONLY);

	while(1)
	{
		while((len1 = read(fd1, &c1, 1)) > 0){
			if(c1 == ' ') // 공백은 무시
				continue;
			else 
				break;
		}
		while((len2 = read(fd2, &c2, 1)) > 0){
			if(c2 == ' ') // 공백은 무시
				continue;
			else 
				break;
		}
		
		if(len1 == 0 && len2 == 0) // 둘 다 파일의 끝이라면 비교 끝
			break;

		to_lower_case(&c1); // 소문자로 바꾸기
		to_lower_case(&c2);

		if(c1 != c2){ // 만약 비교 중, 두개가 다르다면 false 리턴
			close(fd1);
			close(fd2);
			return false;
		}
	}
	close(fd1);
	close(fd2);
	return true; // 두 파일이 같다 - true 리턴
}

void redirection(char *command, int new, int old) // 리디렉션
{
	int saved; // old 복원용

	saved = dup(old); // old fd를 저장
	dup2(new, old); // 리디렉션 old결과는 new로...

	system(command); // 실행

	dup2(saved, old); // old를 복원
	close(saved);
}

int get_file_type(char *filename)
{
	char *extension = strrchr(filename, '.');
	// 마지막 . 찾아서 포인터 리턴

	if(!strcmp(extension, ".txt"))
		return TEXTFILE; // .txt인 경우에 TEXTFILE 리턴
	else if (!strcmp(extension, ".c"))
		return CFILE; // .c 인 경우에 CFILE 리턴
	else
		return -1; // .c .txt 가 아니라면 -1 리턴
}

void rmdirs(const char *path)
{
	struct dirent *dirp;
	struct stat statbuf;
	DIR *dp;
	char tmp[50];
	
	if((dp = opendir(path)) == NULL) // path를 열지 못했다면 즉, 디렉토리가 아니었다면
		return;

	while((dirp = readdir(dp)) != NULL)
	{
		if(!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
			continue;
		// .이랑 .. 은 제외

		sprintf(tmp, "%s/%s", path, dirp->d_name);
		// tmp가 새로운 pathname으로... 문자열 붙이기

		if(lstat(tmp, &statbuf) == -1)
			continue;
		// lstat 에러시에 다음 파일

		if(S_ISDIR(statbuf.st_mode))
			rmdirs(tmp);
		// 디렉토리라면 재귀적으로 삭제
		else
			unlink(tmp);
		// 디렉토리가 아니라면 unlink를 통해 삭제
	}

	closedir(dp);
	rmdir(path);
	// 다 수행하고 자기 자신도 삭제
}

void to_lower_case(char *c) // 대문자들을 소문자로 바꿔주는 함수
{
	if(*c >= 'A' && *c <= 'Z') // 대문자인지 확인함
		*c = *c + 32; // +32 해서 소문자로 바꿔줬음
}

// Usage
void print_usage()
{
	printf("Usage : ssu_score <STUDENTDIR> <TRUEDIR> [OPTION]\n");
	printf("Option : \n");
	printf(" -n <CSVFILENAME>      modify csv file's name\n");
	printf(" -m                    modify question's score\n");
	printf(" -c [STUDENTIDS ...]   print ID's score sum\n");
	printf(" -p [STUDENTIDS ...]   print ID's wrong questions\n");
	printf(" -s <CATEGORY> <1|-1>  sort csv file by <CATEGORY> <1|-1>\n");
	printf("                       <stdid | score> <1:incremental|-1:decremental>\n");
	printf(" -t <QNAMES ...>       compile QNAME.C with -lpthread option\n");
	printf(" -e <DIRNAME>          print error on 'DIRNAME/ID/qname_error.txt' file \n");
	printf(" -h                    print usage\n");
}