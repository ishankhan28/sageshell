<<<<<<< HEAD
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
=======
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <signal.h>	// Handling exits
#include <termios.h>	// For Raw Mode

// Macros
#define FFLAG (O_WRONLY|O_CREAT|O_TRUNC)
#define FMODE (S_IRUSR|S_IWUSR)
#define MAX_BUFFER 	256
#define MAX_CMDS 	10000
#define MAX_MATCHES 	100

// Modified commands
#define LOGS 	"logs"
#define QUIT 	"quit"
#define CLEAR 	"clear"

// Macros for colors

// Basic colors
#define COLOR_RESET	"\033[0m"
#define COLOR_GREEN	"\033[1;32m"
#define COLOR_RED	"\033[1;31m"
#define COLOR_YELLOW	"\033[1;93m"
#define COLOR_BLUE	"\033[1;94m"
#define COLOR_CYAN	"\033[1;36m"
#define COLOR_MAGENTA	"\033[1;35m"

// Ghost text colors
#define COLOR_GREY 	"\033[90m"
#define COLOR_DIM 	"\033[2m"
#define COLOR_GHOST 	"\033[2m\033[90m"		// DIM + GREY COLOR

// Clear line
#define CLEAR_LINE 	"\r\033[K"

// Cursor Manipulation
#define SAVE_CURSOR    "\033[s"
#define RESTORE_CURSOR "\033[u"
#define CLEAR_EOL      "\033[K"
#define CLEAR_BELOW    "\033[J"

// Custom Struct
typedef struct {	// Resource Tracking Purpose
    struct timeval start;
    struct rusage before;
} ResourceTracker;

struct termios original;	// Raw Mode

// Global variables
extern char* known_cmds[MAX_CMDS];
extern int cmd_count;

extern char history[1000][MAX_BUFFER];
extern int history_count;

extern char* matches[MAX_MATCHES];
extern int match_count;
extern int match_index;
extern int cycling_mode;

extern char original_prefix[MAX_BUFFER];


// Basic shell implementation
void executeCmd(char*);
int getArgv(const char*, const char*, char***);
void executeRedirect(char*, int, int);
int parseAndRedirectIn(char*);
int parseAndRedirectOut(char*);
static void perror_exit(char*);
void printBanner();

// Logs related
void maintainLogs(char*);
void displayLogs();

// Better error messages
void loadCommands();
void suggestCommand(char*);
int editDistance(char*, char*);
int min3(int, int, int);
int exists(char*);

// Resource Aware Execution
void startMeasurement(ResourceTracker*, int);
void endMeasurement(ResourceTracker*, int);

// Safety Checks
int isDangerous(char*);
int confirmExecution();


// Raw Mode:
void handleExit(int);		// Handles exit
void enableRawMode();
void disableRawMode();

// Auto-suggestions
void showSuggestions(char*);

// Ghost Text
char* getSuggestion(char*);
void clearMatches();
void buildMatches(char*);
void renderGhost(char*);
void handleTab(char*, int*);
void exitCycling();
>>>>>>> c21b8e4 (Update: added autocomplete, ghost text, and suggestion system)
