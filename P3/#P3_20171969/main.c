#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ssu_monitor.h"

int main(int argc, char *argv[])
{
	ssu_monitor(argc, argv);

	exit(0);
}