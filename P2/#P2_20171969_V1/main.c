#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "ssu_score.h"

#define SECOND_TO_MICRO 1000000

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t);

int main(int argc, char *argv[])
{
	struct timeval begin_t, end_t;
	
	gettimeofday(&begin_t, NULL);
	// 채점 프로그램이 시작 하기 전에 시간을 begin_t에 저장

	ssu_score(argc, argv);
	// 채점 프로그램 작동 - main의 인자 넘겨줌

	gettimeofday(&end_t, NULL);
	// 채점 프로그램이 끝나고 나서 시간을 end_t에 저장
	
	ssu_runtime(&begin_t, &end_t);
	// 소요시간 출력하는 함수 호출

	exit(0);
}

void ssu_runtime(struct timeval *begin_t, struct timeval *end_t)
{
	end_t->tv_sec -= begin_t->tv_sec;
	// end_t->tv_sec - begin_t->tv_sec = 종료시간 - 시작시간

	if(end_t->tv_usec < begin_t->tv_usec){
		end_t->tv_sec--;
		end_t->tv_usec += SECOND_TO_MICRO;
	}

	end_t->tv_usec -= begin_t->tv_usec;
	printf("Runtime: %ld:%06d(sec:usec)\n", end_t->tv_sec, end_t->tv_usec); // %06ld
	// type 'long' but the argument has type '__darwin_suseconds_t'
	// end_t->tv_usec %06ld -> %06d
}
