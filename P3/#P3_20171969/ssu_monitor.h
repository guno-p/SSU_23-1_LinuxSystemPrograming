#ifndef MAIN_H_
#define MAIN_H_

#ifndef true
	#define true 1
#endif

#ifndef false
	#define false 0
#endif

#define FILELEN 128
#define BUFLEN 1024

typedef struct tree {
    char path[BUFLEN];
    mode_t mode;
    time_t mtime;
    struct tree *next;
    struct tree *prev;
    struct tree *child;
    struct tree *parent;
    int isEnd;
} tree;

void ssu_monitor(int argc, char *argv[]);
void ssu_prompt(void);
int execute_command(int argc, char *argv[]);
void execute_add(int argc, char *argv[]);
void execute_delete(int argc, char *argv[]);

void execute_tree(int argc, char *argv[]);
void __execute_tree(tree *node, int level);

void execute_help(int argc, char *argv[]);
void execute_exit(int argc, char *argv[]);
void init_daemon(char *dirpath, time_t mn_time);
pid_t make_daemon(void);

tree *create_node(char *path, mode_t mode, time_t mtime);
void make_tree(tree *dir, char *path);
void compare_tree(tree *old, tree *new);
void free_tree(tree *cur);
void signal_handler(int signum);
int scandir_filter(const struct dirent *file);

void help();
int check_path(const char* reference_path, const char* path_to_check);

#endif