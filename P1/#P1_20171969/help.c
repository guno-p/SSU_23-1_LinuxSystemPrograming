#include "myheader.h"

int main(void)
{
    printf("Usage:\n");
    printf("\t> add <FILENAME> [OPTION]\n");
    printf("\t\t-d : add directory recursive\n");
    printf("\t> remove <FILENAME> [OPTION]\n");
    printf("\t\t-a : remove all file(recursive)\n");
    printf("\t\t-c : clear backup directory\n");
    printf("\t> recover <FILENAME> [OPTION]\n");
    printf("\t\t-d : recover directory recursive\n");
    printf("\t\t-n <NEWNAME> : recover file with new name\n");
    printf("\t> ls\n");
    printf("\t> vi\n");
    printf("\t> vim\n");
    printf("\t> help\n");
    printf("\t> exit\n");    
    exit(0);
}