#include "myheader.h"

void print_help();

int main(int argc,char *argv[])
{
	if (argc != 2 || (strcmp(argv[1], "md5") != 0 && strcmp(argv[1], "sha1") != 0)) {
        fprintf(stderr, "Usage: ssu_backup <md5 | sha1>\n");
        exit(1);
    }

    if((strcmp(argv[1], "md5") == 0)) setenv("HASH_USE", "MD5", 1);
    else if ((strcmp(argv[1], "sha1") == 0)) setenv("HASH_USE", "SHA1", 1);
    // 해시 세팅

    char cwd[1024];
    getcwd(cwd, sizeof(cwd));

    char *homedir = getenv("HOME");
    chdir(homedir);

	if (access("backup", F_OK) < 0) { // don't exist - mkdir
		mkdir("backup", 0777);
	}

    char *path = getenv("PATH");
    char new_path[2048];
    snprintf(new_path, sizeof(new_path), "%s:%s", path, cwd);
    setenv("PATH", new_path, 1);

    chdir(cwd);

    while (1) {
		printf("20171969>");
		char cmd[6000];
        fgets(cmd, 6000, stdin);
        cmd[strcspn(cmd, "\n")] = '\0';
        // add exam -d\n
        // 012345678910 cmd[11] = NULL
        int len = strlen(cmd);

        // Check if the input is empty - input nothing
        if (len == 0) {
            continue;
        }

        if (cmd[0] == (char)32) { // spacebar - segmentation fault handling
            print_help();
            continue;
        }
        
        // tokenize cmd
        char *token;
        char *args[30]; // Assuming maximum of 30 words
        for (int i = 0; i < 30; i++) {
            args[i] = NULL; // Filling every element with NULL
        }

        token = strtok(cmd, " ");
        int i=0;
        while(token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }

		if (strcmp(args[0], "add") == 0) {
            int pid, status;
            // Fork a new process
            pid = fork();
            if (pid < 0) { // Error
                printf("Fork error\n");
                exit(1);
            }
            else if (pid == 0) { // Child process
                // Execute the add program
                execvp(args[0], args);
                // If execvp returns, an error occurred
                printf("Error executing add command\n");
                exit(1);
            }
            else { // Parent process
                // Wait for child process to finish
                waitpid(pid, &status, 0);
            }
        }

		else if (strcmp(args[0], "remove") == 0) {
            int pid, status;
            // Fork a new process
            pid = fork();
            if (pid < 0) { // Error
                printf("Fork error\n");
                exit(1);
            }
            else if (pid == 0) { // Child process
                // Execute the remove program
                execvp(args[0], args);
                // If execvp returns, an error occurred
                printf("Error executing remove command\n");
                exit(1);
            }
            else { // Parent process
                // Wait for child process to finish
                waitpid(pid, &status, 0);
            }
            // Remove file from backup
        }

		else if (strcmp(args[0], "recover") == 0) {
            int pid, status;
            // Fork a new process
            pid = fork();
            if (pid < 0) { // Error
                printf("Fork error\n");
                exit(1);
            }
            else if (pid == 0) { // Child process
                // Execute the recover program
                execvp(args[0], args);
                // If execvp returns, an error occurred
                printf("Error executing recover command\n");
                exit(1);
            }
            else { // Parent process
                // Wait for child process to finish
                waitpid(pid, &status, 0);
            }
            // Recover file from backup
        }

		else if (strcmp(args[0], "ls") == 0) {
            int pid, status;
            // Fork a new process
            pid = fork();
            if (pid < 0) { // Error
                printf("Fork error\n");
                exit(1);
            }
            else if (pid == 0) { // Child process
                // Execute the ls program
                execvp(args[0], args);
                // If execvp returns, an error occurred
                printf("Error executing ls command\n");
                exit(1);
            }
            else { // Parent process
                // Wait for child process to finish
                waitpid(pid, &status, 0);
            }
        }

		else if ((strcmp(args[0], "vi") == 0) || (strcmp(args[0], "vim") == 0)) {
            int pid, status;
            // Fork a new process
            pid = fork();
            if (pid < 0) { // Error
                printf("Fork error\n");
                exit(1);
            }
            else if (pid == 0) { // Child process
                // Execute the vim program
                execvp(args[0], args);
                // If execvp returns, an error occurred
                printf("Error executing vim command\n");
                exit(1);
            }
            else { // Parent process
                // Wait for child process to finish
                waitpid(pid, &status, 0);
            }
        }

		else if (strcmp(args[0], "help") == 0) {
		    int pid, status;
            // Fork a new process
            pid = fork();
            if (pid < 0) { // Error
                printf("Fork error\n");
                exit(1);
            }
            else if (pid == 0) { // Child process
                // Execute the help program
                execvp(args[0], args);
                // If execvp returns, an error occurred
                printf("Error executing help command\n");
                exit(1);
            }
            else { // Parent process
                // Wait for child process to finish
                waitpid(pid, &status, 0);
            }
        }

		else if (strcmp(args[0], "exit") == 0) {
            break; // input exit is only way to end loop
    	}

        else {
            print_help();
        }
	}
	exit(0);
}

void print_help()
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
}