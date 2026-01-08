#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
#include<errno.h>
#define MAX_BUFFER 256
#define QUIT_STRING "quit"
#include<fcntl.h>
#include<sys/stat.h>
#define FFLAG (O_WRONLY|O_CREAT|O_TRUNC)
#define FMODE (S_IRUSR|S_IWUSR)
void executeredirect(char*,int,int);
int getargv(const char*,const char*,char***);
int parseandredirectin(char*);
int parseandredirectout(char*);
void executecmd(char*);
void printBanner();
