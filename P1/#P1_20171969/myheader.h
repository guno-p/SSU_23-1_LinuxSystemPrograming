#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <libgen.h>
#define OPENSSL_API_COMPAT 0x10100000L//MD5 is deprecated so USE OPENSSL_API_COMPAT...
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <locale.h>// for 1,000,000