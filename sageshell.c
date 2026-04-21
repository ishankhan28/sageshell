#include "sageshell.h"

char* known_cmds[MAX_CMDS];
int cmd_count = 0;

char history[1000][MAX_BUFFER];
int history_count = 0;

char* matches[MAX_MATCHES];
int match_count = 0;
int match_index = 0;
int cycling_mode = 0;

char original_prefix[MAX_BUFFER];

static void perror_exit(char* s){
	perror(s);
	_exit(1);
}

int main(int argc, char* argv[]){

	setbuf(stdout, NULL);

	enableRawMode();

	atexit(disableRawMode);
	signal(SIGINT, handleExit);

	char** chargv;
	char inbuf[MAX_BUFFER];
	pid_t pid;

	inbuf[0] = '\0';

	// LOADS COMMANDS DYNAMICALLY FOR ERROR CHECKING
	loadCommands();		// loadCommands() invoked...

	// Custom commands added to known_cmds array	Better Error Messages
	if(cmd_count < MAX_CMDS)
		known_cmds[cmd_count++] = strdup("logs");
	if(cmd_count < MAX_CMDS)
		known_cmds[cmd_count++] = strdup("quit");

	printBanner();		// PRINTS BANNER

	while(1){
		fputs(COLOR_BLUE"sageshell$ "COLOR_RESET, stdout);	// Puts "sageshell$ " on the command line

		// Storing input using Raw Mode(ECHO is OFF)
		int i = 0;
		char c;

		while(1) {

    			c = getchar();

			// Handling arrow keys
			if(c == 27) {
                            char seq1 = getchar();
                            char seq2 = getchar();

                            // 👉 RIGHT ARROW
                            if(seq1 == '[' && seq2 == 'C') {

                                char* suggestion = getSuggestion(inbuf);

                                if(suggestion) {
                                    strcpy(inbuf, suggestion);
                                    i = strlen(inbuf);
                                }

                                // 🔥 REDRAW AFTER ACCEPT
                                printf("\r");
                                printf(CLEAR_LINE);
                                printf(COLOR_BLUE "sageshell$ " COLOR_RESET "%s", inbuf);
                                renderGhost(inbuf);

                                fflush(stdout);
                            }

                            continue;
                        }

			if(c == EOF) {
				system("clear");
				printf("\nExiting sageshell...\n");
				exit(0);
			}

			if(c == '\t') {
    				handleTab(inbuf, &i);		// handleTab() invoked...
				printf("\r");
				printf(CLEAR_LINE);
				printf(COLOR_BLUE "sageshell$ " COLOR_RESET "%s", inbuf);
				renderGhost(inbuf);
				fflush(stdout);
				continue;
			}

    			if(c == '\n') {
    				printf("\n");
    				break;
			}

    			if(c == 127 || c == '\b') {
				exitCycling();		// exitCycling() invoked...
        			if(i > 0) {
            				i--;
            				inbuf[i] = '\0';

					printf("\r");
					printf(CLEAR_LINE);
        				printf(COLOR_BLUE "sageshell$ " COLOR_RESET "%s", inbuf);
        				renderGhost(inbuf);	// renderGhost() invoked...

        			}
        			continue;
    			}

    			if(i < MAX_BUFFER - 1) {
				exitCycling();		// exitCycling() invoked...

        			inbuf[i++] = c;
				inbuf[i] = '\0';

				printf("\r");
				printf(CLEAR_LINE);
    				printf(COLOR_BLUE "sageshell$ " COLOR_RESET "%s", inbuf);
    				renderGhost(inbuf);	// renderGhost() invoked...
    			}
		}

		inbuf[i] = '\0';	// Adds '\0' NULL at length-1

		int onlySpaces = 1;	// Handles only spaces or empty string
		for(int j = 0; inbuf[j]; j++) {
	    		if(inbuf[j] != ' ' && inbuf[j] != '\t') {
        			onlySpaces = 0;
        			break;
    			}
		}

		if(onlySpaces) continue;	// Continues if empty string or only spaces are present...

		// Maintaining history for each command in 'logFile.txt'
		if(inbuf[0]!='\0' && inbuf[0]!=' ' && inbuf[0]!='\n' && inbuf[0]!='\t') maintainLogs(inbuf);

		if(history_count < 1000) {
			strcpy(history[history_count++], inbuf);
		}

		// Handling 'logs' command...
		if(strcmp(inbuf, LOGS)==0) {
			ResourceTracker rt;	// Structure type variable declared
			startMeasurement(&rt, 1);	// startMeasurement() invoked...
			displayLogs();
			endMeasurement(&rt, 1);		// endMeasurement() invoked...
			continue;
		}

		// Handling 'clear' command...
		if(strcmp(inbuf, CLEAR)==0) {
			system("clear");
			printBanner();
			continue;
		}

		// Handling 'quit' command...
		if(strcmp(inbuf, QUIT)==0) {
			system("clear");
			printf("\nExiting sageshell...\n");
			exit(0);
		}

		// Forking is done and main process will be divided in 2 processes

		ResourceTracker rt;	// Structure type variable declared
		startMeasurement(&rt, 0);	// Children		startMeasurement() invoked...

		fflush(stdout);

		switch(pid=fork()) {
			case -1:
				printf("Something Went Wrong\n");
				break;
			case 0:
				executeCmd(inbuf);
				_exit(1);
			default:
				int status;
    				wait(&status);

				if(WIFEXITED(status) && WEXITSTATUS(status) == 127) {	// Suggestion logic
        				char cmdCopy[MAX_BUFFER];
        				strcpy(cmdCopy, inbuf);
        				char* cmd = strtok(cmdCopy, " \t");
        				if(cmd != NULL) {
            					suggestCommand(cmd);	// suggestCommand() invoked...
        				}
    				}

				endMeasurement(&rt, 0);		// endMeasurement() invoked...

				break;
		}
	}
	return 0;
}

void executeCmd(char* cmd){

	if(isDangerous(cmd)) {		// Safety Check implemented...		isDangerous invoked...
		if(!confirmExecution()) {	// confirmExecution() invoked...
		        printf(COLOR_YELLOW"Command aborted.\n"COLOR_RESET);
			return;
		} else {
			printf(COLOR_YELLOW"✔ Executing dangerous command...\n"COLOR_RESET);
		}
	}

	int child, count, i;
	int fds[2];
	int prev_fd = -1;
	char** chargv;
	pid_t pid;
	char** pipelist;

	count = getArgv(cmd, "|", &pipelist);

	if(count <= 0) {
		fprintf(stderr, "Failed to find any argument\n");
		return;
	}

	// PIPELINE
	if(count > 1) {

		for(i = 0; i < count; i++) {

			// Create pipes for all commands EXCEPT last
			if(i < count - 1) {
				if(pipe(fds) == -1) {
					perror_exit("Failed to create pipes\n");
				}
			}

			if((child = fork()) == -1) {
				perror_exit("Failed to create process\n");
			}

			else if(child == 0) {

				// Input from previous pipe
				if(prev_fd != -1) {
					dup2(prev_fd, STDIN_FILENO);
					close(prev_fd);
				}

				// Output to next pipe
				if(i < count - 1) {
					dup2(fds[1], STDOUT_FILENO);
					close(fds[0]);
					close(fds[1]);
				}

				executeRedirect(pipelist[i], i==0, i==count-1);
				_exit(1);
			}

			// Parent
			if(prev_fd != -1)
				close(prev_fd);

			if(i < count - 1) {
				close(fds[1]);
				prev_fd = fds[0];
			}
		}

		// Wait after all forks
		int status;
		for(int j = 0; j < count; j++) {
			if(wait(&status) == -1) {
				perror("wait failed");
			}
		}
		return;
	}

	// NORMAL EXECUTION

	if(parseAndRedirectOut(cmd)==-1)
		perror("Failed to redirect Output\n");

	if(parseAndRedirectIn(cmd)==-1)
		perror("Failed to redirect Input\n");

	if(getArgv(cmd, " \t", &chargv)<=0)
		perror("Failed To Parse Command\n");
	else {
		fflush(stdout);
		execvp(chargv[0], chargv);

		if(errno == ENOENT) {
			fprintf(stderr, COLOR_RED"Command '%s' not found.\n"COLOR_RESET, chargv[0]);
		} else {
			perror("Execution failed");
		}
		_exit(127);
	}
}
int getArgv(const char* s, const char* delims, char*** argvp){
	int error, i, numtoken=0;
	const char* snew;
	char* t;
	if((s==NULL) || (delims==NULL) || (argvp==NULL)) {
		*argvp=NULL;
		return -1;
	}
	snew=s+strspn(s, delims);
        if((t=malloc(strlen(snew)+1))==NULL)
		return -1;
	strcpy(t, snew);
	if(strtok(t, delims)!=NULL) {
		for(numtoken=1; strtok(NULL, delims)!=NULL; numtoken++);
	}
	if((*argvp=malloc((numtoken+1)*sizeof(char*)))==NULL) {
		free(t);
		return -1;
	}
	if(numtoken==0) {
		free(t);
		*argvp = NULL;
    		return 0;
	} else {
		strcpy(t, snew);
		**argvp=strtok(t, delims);
		for(i=1; i<numtoken; i++) {
			*((*argvp)+i)=strtok(NULL, delims);
		}
		*((*argvp)+numtoken)=NULL;
		return numtoken;
	}
}
int parseAndRedirectIn(char* cmd){
	int error, infd;
	char* infile;
	if((infile=strchr(cmd, '<'))==NULL)
		return 0;
	*infile='\0';
	infile=strtok(infile+1, " \t");
	if(infile==NULL)
		return 0;
	if((infd=open(infile, O_RDONLY))==-1)
		return -1;
	if(dup2(infd, STDIN_FILENO/*Terminal File No.*/)==-1) {
		error=errno;
		close(infd);
		errno=error;
		return -1;
	}
	return close(infd);
}

int parseAndRedirectOut(char* cmd){
	int error, outfd;
	char* outfile;
	if((outfile=strchr(cmd, '>'))==NULL)
		return 0;
	*outfile='\0';
	outfile=strtok(outfile+1, " \t");
	if(outfile==NULL)
		return 0;
	if((outfd=open(outfile, FFLAG, FMODE))==-1)
		return -1;
	if((dup2(outfd, STDOUT_FILENO))==-1) {
		error=errno;
		close(outfd);
		errno=error;
		return -1;
	}
	return close(outfd);
}

void executeRedirect(char* s, int in, int out) {

    	char** chargv;

    	// Handle output redirection
    	if(parseAndRedirectOut(s) == -1) {
        	perror("Failed to redirect output");
        	_exit(1);
    	}

    	// Handle input redirection
    	if(parseAndRedirectIn(s) == -1) {
        	perror("Failed to redirect input");
        	_exit(1);
    	}

    	// Parse arguments
    	if(getArgv(s, " \t", &chargv) <= 0) {
        	fprintf(stderr, "Failed to parse command\n");
        	_exit(1);
    	}

    	// Execute command
    	execvp(chargv[0], chargv);

    	// ONLY IF execvp FAILS

    	if(errno == ENOENT) {
        	fprintf(stderr, "Command '%s' not found.\n", chargv[0]);
    	} else {
        	perror("Execution failed");
    	}

    	_exit(1);   // ALWAYS terminate child
}

void printBanner() {
	system("clear");
    	printf("\n");
    	printf(COLOR_CYAN"╔═══════════════════════════════════════════════════════════════╗\n"COLOR_RESET);
    	printf(COLOR_CYAN"║        ███████╗ █████╗  ██████╗ ███████╗     Shell v1.0       ║\n"COLOR_RESET);
    	printf(COLOR_CYAN"║        ██╔════╝██╔══██╗██╔════╝ ██╔════╝      Raw mode        ║\n"COLOR_RESET);
    	printf(COLOR_CYAN"║        ███████╗███████║██║  ███╗█████╗      Autocomplete      ║\n"COLOR_RESET);
    	printf(COLOR_CYAN"║        ╚════██║██╔══██║██║   ██║██╔══╝       Ghost text       ║\n"COLOR_RESET);
    	printf(COLOR_CYAN"║        ███████║██║  ██║╚██████╔╝███████╗                      ║\n"COLOR_RESET);
    	printf(COLOR_CYAN"║        ╚══════╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝     SAGE Shell       ║\n"COLOR_RESET);
    	printf(COLOR_CYAN"╚═══════════════════════════════════════════════════════════════╝\n"COLOR_RESET);
    	printf("\n");
    	printf(COLOR_MAGENTA"           Suggestive Auto-Command Generative Environment\n"COLOR_RESET);
	printf("  Use  "COLOR_MAGENTA"TAB"COLOR_RESET"  for suggestions      |  "COLOR_MAGENTA"→"COLOR_RESET"    to accept ghost text\n");
	printf("  Type "COLOR_MAGENTA"logs"COLOR_RESET" for command history  |  "COLOR_MAGENTA"quit"COLOR_RESET" to quit sage\n\n");
}

void maintainLogs(char inbuf[]) {
	FILE* file=fopen("logFile.txt", "a+");
	int i=0;
	while(i<strlen(inbuf))
		fputc(inbuf[i++], file);
	fputc(10, file); // Adds new line to each command
	fclose(file);
}

void displayLogs() {
	printf(COLOR_CYAN"Command History:\n");
	FILE* file=fopen("logFile.txt", "r");
	int ch;
	while((ch=fgetc(file))!=EOF)
		fputc(ch, stdout);
	fclose(file);
	printf(COLOR_RESET);
}

// ---------------------------------------------------

// Feature Added: Better Error Messages

int exists(char* cmd) {		// loadCommands() invokes it
	for(int i = 0; i < cmd_count; i++) {
        	if(strcmp(known_cmds[i], cmd) == 0)
            	return 1;
    	}
    	return 0;
}

void loadCommands() {		// executeCmd() invokes it
    	char* path = getenv("PATH");
	if(!path) return;
    	char* path_copy = strdup(path);

    	char* dir = strtok(path_copy, ":");

    	while(dir != NULL) {
        	DIR* d = opendir(dir);

        	if(d) {
            		struct dirent* entry;

            		while((entry = readdir(d)) != NULL) {

                		char fullpath[512];
                		snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, entry->d_name);

                		struct stat st;

                		if(stat(fullpath, &st) == 0 && (st.st_mode & S_IXUSR)) {

                    			if(!exists(entry->d_name)) {		// exists() invoked...
                        			if(cmd_count < MAX_CMDS)
							known_cmds[cmd_count++] = strdup(entry->d_name);

                        			if(cmd_count >= MAX_CMDS) break;
                    			}
                		}
            		}
            		closedir(d);
        	}
        	dir = strtok(NULL, ":");
    	}
    	free(path_copy);
}

int min3(int a, int b, int c) {		// editDistance() invokes it
    	if(a <= b && a <= c) return a;
    	if(b <= a && b <= c) return b;
    	return c;
}

int editDistance(char* a, char* b) {		// suggestCommand() invokes it
    	int lenA = strlen(a), lenB = strlen(b);
    	int dp[100][100];

    	for(int i = 0; i <= lenA; i++) dp[i][0] = i;
    	for(int j = 0; j <= lenB; j++) dp[0][j] = j;

    	for(int i = 1; i <= lenA; i++) {
        	for(int j = 1; j <= lenB; j++) {
            		if(a[i-1] == b[j-1])
                		dp[i][j] = dp[i-1][j-1];
            		else
                		dp[i][j] = 1 + min3(dp[i-1][j], dp[i][j-1], dp[i-1][j-1]);	// min3() invoked
        	}
    	}
    	return dp[lenA][lenB];
}

void suggestCommand(char* input) {	// executeCmd(), executeRedirect() invokes it

    	int bestScore = 100;
    	char* bestMatch = NULL;

    	for(int i = 0; i < cmd_count; i++) {

        	int dist = editDistance(input, known_cmds[i]);

        	// bonus score for matching characters
        	int matchScore = 0;

        	for(int j = 0; j < strlen(input); j++) {
            		if(strchr(known_cmds[i], input[j]))
                		matchScore++;
        	}

        	// final score (lower is better)
        	int finalScore = dist - matchScore;

        	if(finalScore < bestScore) {
            		bestScore = finalScore;
            		bestMatch = known_cmds[i];
        	}
    	}

    	if(bestMatch && bestScore <= 2) {
        	printf("Did you mean '%s'?\n", bestMatch);
    	}
}

// ---------------------------------------------------

// Feature Added: Resource Aware Execution

void startMeasurement(ResourceTracker* rt, int type) {		// main() invokes it
    	gettimeofday(&rt->start, NULL);
	if(type == 0)
        	getrusage(RUSAGE_CHILDREN, &rt->before);
    	else
        	getrusage(RUSAGE_SELF, &rt->before);
}

void endMeasurement(ResourceTracker* rt, int type) {		// main() invokes it
    	struct timeval end;
    	struct rusage after;

    	gettimeofday(&end, NULL);
    	if(type == 0)
        	getrusage(RUSAGE_CHILDREN, &after);
    	else
        	getrusage(RUSAGE_SELF, &after);

	// Execution Time Calculated
    	double exec_time = (end.tv_sec - rt->start.tv_sec) + (end.tv_usec - rt->start.tv_usec) / 1000000.0;

	// CPU Time Calculated
    	double cpu_time =	(after.ru_utime.tv_sec - rt->before.ru_utime.tv_sec) +
        			(after.ru_utime.tv_usec - rt->before.ru_utime.tv_usec) / 1000000.0 +
        			(after.ru_stime.tv_sec - rt->before.ru_stime.tv_sec) +
			        (after.ru_stime.tv_usec - rt->before.ru_stime.tv_usec) / 1000000.0;

	// Memory Usage Calculated
	double mem = (after.ru_maxrss - rt->before.ru_maxrss) / 1024.0;
    	if(mem < 0) mem = 0;

    	printf(COLOR_GREEN"\n[Time: %.4fs | CPU: %.4fs | Memory: %.2f MB]\n"COLOR_RESET, exec_time, cpu_time, mem);
}

// ---------------------------------------------------

// Feature Added: Safety Checks

int isDangerous(char* cmd) {	// executeCmd() invokes it
    	char temp[MAX_BUFFER];
    	strcpy(temp, cmd);   // work on copy

    	char* args[50];
    	int argc = 0;

    	char* token = strtok(temp, " \t");

    	while(token != NULL && argc < 49) {
        	args[argc++] = token;
        	token = strtok(NULL, " \t");
    	}
    	args[argc] = NULL;

    	if(argc == 0) return 0;

    	// Check ONLY actual command
    	if(strcmp(args[0], "rm") != 0)
        	return 0;

    	int recursive = 0;
    	int force = 0;

    	// Parse flags
    	for(int i = 1; i < argc; i++) {
        	if(args[i][0] == '-') {

			if(strchr(args[i], 'r')) recursive = 1;
            		if(strchr(args[i], 'f')) force = 1;
		}
    	}

    	// Check dangerous targets
    	for(int i = 1; i < argc; i++) {

        	// skip flags
        	if(args[i][0] == '-') continue;

        	// CRITICAL: root deletion
        	if(strcmp(args[i], "/") == 0)
            		return 1;

        	// current directory
        	if(strcmp(args[i], ".") == 0 && recursive)
            		return 1;

        	// wildcard deletion
        	if(strchr(args[i], '*') != NULL)
            		return 1;
    	}

    	// rm -rf without target (very risky behavior)
    	if(recursive && force && argc == 1)
        	return 1;

	// Detect rm -rf (any target)
	if(recursive && force)
    		return 1;

    	return 0;
}

int confirmExecution() {	// executeCmd() invokes it
    	char c;
    	printf(COLOR_YELLOW"⚠ Warning: This command may be dangerous.\n");
    	printf("Do you want to continue? (y/n): "COLOR_RESET);
    	while(1) {
		c = getchar();
		if(c == 'y') {
			printf("y\n");
			return 1;
		} else if(c == 'Y') {
			printf("Y\n");
			return 1;
		} else if(c == 'n') {
			printf("n\n");
			return 0;
		} else if(c == 'N') {
			printf("N\n");
			return 0;
		}
	}
}

// ---------------------------------------------------

// Raw Mode

void handleExit(int sig) {	// Handles exit		inside main() signal() invokes it
	system("clear");
	printf("\nExiting sageshell...\n");
	disableRawMode();	// disableRawMode() invoked...
	_exit(0);
}

void enableRawMode() {		// enableRawMode
    	struct termios raw;

    	tcgetattr(STDIN_FILENO, &original);  // save current settings
    	raw = original;

    	raw.c_lflag &= ~(ICANON | ECHO);     // disable canonical mode + echo

    	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disableRawMode() {		// handleExit() invokes it
    	tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
}

// ---------------------------------------------------

// Feature Added: Auto-suggestions

void showSuggestions(char* input) {

    	clearMatches();   // IMPORTANT

    	char temp[MAX_BUFFER];
    	strcpy(temp, input);

    	char* last = strrchr(temp, ' ');
    	char* prefix = last ? last + 1 : temp;

    	int prefix_len = strlen(prefix);
    	if(prefix_len < 1) return;

    	int isCommand = (last == NULL);
    	int printed = 0;
    	const int LIMIT = 8;

    	printf("\n");

    	// COMMAND MODE (FIRST WORD)
    	if(isCommand) {

        	// STEP 1: HISTORY FIRST
        	for(int i = history_count - 1; i >= 0 && printed < LIMIT; i--) {

            		if(strncmp(history[i], prefix, prefix_len) == 0) {

                		char cmd[100];
                		sscanf(history[i], "%s", cmd);

                		int duplicate = 0;
                		for(int j = 0; j < printed; j++) {
                    			if(strcmp(cmd, matches[j]) == 0) {
                        			duplicate = 1;
                       				break;
                    			}
                		}

                		if(!duplicate) {
                    			printf("  %s\n", cmd);
                    			matches[printed++] = strdup(cmd);
                		}
            		}
        	}

        	// STEP 2: COMMANDS FROM PATH
        	for(int i = 0; i < cmd_count && printed < LIMIT; i++) {

            		if(strncmp(known_cmds[i], prefix, prefix_len) == 0) {

                		int duplicate = 0;
                		for(int j = 0; j < printed; j++) {
                    			if(strcmp(known_cmds[i], matches[j]) == 0) {
                        			duplicate = 1;
                        			break;
                    			}
                		}

                		if(!duplicate) {
                 	   		printf("  %s\n", known_cmds[i]);
                    			matches[printed++] = strdup(known_cmds[i]);
                		}
            		}
        	}
    	}

    	// ARGUMENT MODE (FILES IN PWD)
    	else {

        	DIR* d = opendir(".");
        	struct dirent* entry;

        	if(d) {
            		while((entry = readdir(d)) != NULL && printed < LIMIT) {

                		if(strncmp(entry->d_name, prefix, prefix_len) == 0) {
                   	 		printf("  %s\n", entry->d_name);
                    			printed++;
                		}
            		}
            		closedir(d);
        	}
    	}

    	// FINAL OUTPUT
    	if(printed == 0) {
        	printf("  No matches\n");
    	}
    	else if(printed == LIMIT) {
        	printf("  ... more\n");
    	}

    	// REDRAW PROMPT CLEANLY
    	printf("\r");
    	printf(CLEAR_LINE);
    	printf(COLOR_BLUE "sageshell$ " COLOR_RESET "%s", input);

    	renderGhost(input);
    	fflush(stdout);
}

// ---------------------------------------------------

// Feature Added: Inline Suggestions with GHOST TEXT

char* getSuggestion(char* input) {	// printGhost(), renderGhost() and Input loop inside main() invokes it
    	int len = strlen(input);
    	if(len == 0) return NULL;
    	static char suggestion[256];

    	// detect context
    	char* last = strrchr(input, ' ');
    	int isCommand = (last == NULL);
    	char* prefix = last ? last + 1 : input;
    	int plen = strlen(prefix);

    	// HISTORY FIRST
    	for(int i = history_count - 1; i >= 0; i--) {
        	if(strncmp(history[i], input, len) == 0 &&
           	strcmp(history[i], input) != 0) {
            	strcpy(suggestion, history[i]);
            	return suggestion;
        	}
    	}

    	// COMMAND MODE
    	if(isCommand) {
        	for(int i = 0; i < cmd_count; i++) {
            		if(strncmp(known_cmds[i], prefix, plen) == 0) {
                		strcpy(suggestion, known_cmds[i]);
                		return suggestion;
            		}
        	}
    	}

    	// FILE MODE
    	else {
        	DIR* d = opendir(".");
        	struct dirent* entry;
        	if(d) {
           		while((entry = readdir(d)) != NULL) {
                		if(strncmp(entry->d_name, prefix, plen) == 0) {

                    			// rebuild full command
                    			if(last) {
                        			int offset = last - input + 1;
                        			strncpy(suggestion, input, offset);
                        			suggestion[offset] = '\0';
                        			strcat(suggestion, entry->d_name);
                    			} else {
                        			strcpy(suggestion, entry->d_name);
                    			}
                    			closedir(d);
                    			return suggestion;
                		}
            		}
            		closedir(d);
        	}
    	}
    	return NULL;
}

void clearMatches() {		// buildMatches(), handleTab(), exitCycling() invokes it
    	for(int i = 0; i < match_count; i++)
        	free(matches[i]);
    	match_count = 0;
}

void buildMatches(char* inbuf) {	// handleTab() invokes it

    	clearMatches();   // always reset previous matches
    	char* last = strrchr(inbuf, ' ');
    	char* prefix = last ? last + 1 : inbuf;
    	int plen = strlen(prefix);

    	// avoid noisy suggestions
    	if(plen < 2) return;
    	int isCommand = (last == NULL);

    	// COMMAND MODE (FIRST WORD)
    	if(isCommand) {
        	for(int i = 0; i < cmd_count; i++) {
            		if(strncmp(known_cmds[i], prefix, plen) == 0) {
                		if(match_count < MAX_MATCHES) {
                    			matches[match_count++] = strdup(known_cmds[i]);
                		} else {
                    			break;
                		}
            		}
        	}
    	}

    	// FILE MODE (ARGUMENTS)
    	else {
		DIR* d = opendir(".");
        	struct dirent* entry;
	        if(d) {
        	    	while((entry = readdir(d)) != NULL) {
                		if(strncmp(entry->d_name, prefix, plen) == 0) {
                    			char* full = malloc(MAX_BUFFER);
                    			if(!full) {
                        			clearMatches();
                        			closedir(d);
                        			return;
                    			}
                    			int offset = last - inbuf + 1;

                    			// SAFE COPY (manual)
                    			int i = 0;

                    			// copy first part (command + space)
                    			for(; i < offset && i < MAX_BUFFER - 1; i++) {
                        			full[i] = inbuf[i];
                    			}

                    			// append filename
                    			int j = 0;
                    			while(entry->d_name[j] != '\0' && i < MAX_BUFFER - 1) {
                        			full[i++] = entry->d_name[j++];
                    			}
                    			full[i] = '\0';   // null terminate
                    			if(match_count < MAX_MATCHES) {
                        			matches[match_count++] = full;
                    			} else {
                        			free(full);
                        			break;
                    			}
                		}
            		}
            		closedir(d);
        	}
    	}
}

void renderGhost(char* inbuf) {		// Input loop inside main() invokes it

    	if(strlen(inbuf) < 2) return;

    	char* suggestion = getSuggestion(inbuf);	// getSuggestion() invoked...
    	if(!suggestion) return;

    	int len = strlen(inbuf);
    	if(strlen(suggestion) <= len) return;

    	char* ghost = suggestion + len;

    	printf(SAVE_CURSOR);     // save cursor
    	printf(CLEAR_EOL);       // clear old ghost

    	printf(COLOR_GHOST "%s" COLOR_RESET, ghost);

    	printf(RESTORE_CURSOR);  // restore cursor

    	fflush(stdout);
}

// Tab Handler

void handleTab(char* inbuf, int* len) {		// Input loop inside main() invokes it
	if(strlen(inbuf) == 0) return;

    	if(!cycling_mode) {

        	buildMatches(inbuf);		// buildMatches() invoked...

        	if(match_count == 0) return;

        	if(match_count == 1) {
            		strcpy(inbuf, matches[0]);
            		strcat(inbuf, " ");
            		*len = strlen(inbuf);
            		clearMatches();		// clearMatches() invoked...
            		return;
        	}

        	cycling_mode = 1;
        	match_index = 0;
        	strcpy(original_prefix, inbuf);

        	printf("\n");
		int LIMIT = 8;
		int printed = 0;

		for(int i = 0; i < match_count && printed < LIMIT; i++) {
			printf("  %s\n", matches[i]);
 			printed++;
 		}

		if(match_count > LIMIT) {
			printf("  ... more\n");
		}

		strcpy(inbuf, matches[0]);
		*len = strlen(inbuf);
		return;
	}

    	match_index = (match_index + 1) % match_count;

    	strcpy(inbuf, matches[match_index]);
    	*len = strlen(inbuf);
}

// Exit Cycling Mode

void exitCycling() {		// Input loop inside main() invokes it
    	if(cycling_mode) {
        	cycling_mode = 0;
        	clearMatches();		// clearMatches() invoked...
        	printf("\r");
		printf(CLEAR_LINE);
		printf(CLEAR_BELOW);
    	}
}
