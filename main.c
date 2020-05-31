#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

struct numProcessFail {
	char *procname;
	int num;
};

struct numProcessFailNode {
	struct numProcessFail *curr;
	struct numProcessFailNode *next;
};

struct logEntry {
	char *month, *date, *time, *user, *procinfo, *info;
};

struct logEntryNode {
	struct logEntry *curr;
	struct logEntryNode *next;
};

/* CONSTANTS */
#ifdef __APPLE__
	//Log location for MacOS 
	char *fileName = "/var/log/system.log";
#else
	//Log location for Linux
	char *fileName = "/var/log/syslog"
#endif

char *keys[] = {"bug", "error", "incorrect password"};
int bufferSize = 1000;
FILE *fp = NULL;
int fileOffset = 0;
struct logEntryNode *logEntryList = NULL;
struct numProcessFailNode *numProcessFailList = NULL;

/* HELPER FUNCTIONS */
void openFile() {
	fp = fopen(fileName, "r");
	fseek(fp, fileOffset, SEEK_SET);
}

void closeFile() {
	fclose(fp);
	fp = NULL;
}

void copySubstring(char *src, char *dest, int beg, int end) {
	for (int i = beg; i <= end; i++) {
		dest[i-beg] = src[i];
	}
	dest[(end-beg+1)] = '\0';
}

//month date time user processinfo info
//0		1	 2    3    4           5
struct logEntry* parseEntry(char *line) {
	int len = strlen(line);
	struct logEntry *curr = malloc(sizeof(struct logEntry));
	curr->month = malloc(5 * sizeof(char));
	curr->date = malloc(2 * sizeof(char));
	curr->time = malloc(10 * sizeof(char));
	curr->user = malloc(100 * sizeof(char));
	curr->procinfo = malloc(100 * sizeof(char));
	curr->info = malloc(1000 * sizeof(char)); 

	int beg = 0, index = 0;
	for (int i = 0; i < len; i++) {
		if (index == 4 && line[i] == ':') {
			copySubstring(line, curr->procinfo, beg, i-1);
			beg = i+1;
			index++;
		} else if (index < 4 && line[i] == ' ') {
			if (index == 0)
				copySubstring(line, curr->month, beg, i-1);
			else if (index == 1)
				copySubstring(line, curr->date, beg, i-1);
			else if (index == 2)
				copySubstring(line, curr->time, beg, i-1);
			else
				copySubstring(line, curr->user, beg, i-1);
			beg = i+1;
			index++;
		}
	}

	copySubstring(line, curr->info, beg, len-1);

	return curr;
}

void updateNumProcessFail(struct logEntry *curr) {
	struct numProcessFailNode *temp = numProcessFailList;
	int found = 0;
	while (temp != NULL) {
		if (strcmp(temp->curr->procname, curr->procinfo) == 0) {
			temp->curr->num++;
			found = 1;

			if (temp->curr->num % 5 == 0) {
				printf("%s triggered %d times\n", temp->curr->procname, temp->curr->num);
			}

			break;
		}

		temp = temp->next;
	}

	if (!found) {
		struct numProcessFailNode *currNode = malloc(sizeof(struct numProcessFailNode));
		struct numProcessFail *entry = malloc(sizeof(struct numProcessFail));
		currNode->curr = entry;
		entry->procname = curr->procinfo;
		entry->num = 1;
		currNode->next = numProcessFailList;
		numProcessFailList = currNode;
	}
}

void convertToLower(char* inp) {
	for (int i = 0; i < strlen(inp); i++) {
		inp[i] = tolower(inp[i]);
	}
}

int substringSearch(char *line, char *keyword) {
	int N = strlen(line), M = strlen(keyword);
	for (int i = 0; i < N - M + 1; i++) {
		int found = 1;
		for (int j = 0; j < M; j++) {
			if (line[i+j] != keyword[j]) {
				found = 0;
				break;
			}
		}
		if (found == 1)
			return found;
	}

	return 0;
}

char* readLine() {
	char *line = malloc(bufferSize * sizeof(char));
	line = fgets(line, bufferSize, fp);
	if (line == NULL)
		return NULL;
	fileOffset += strlen(line);
	convertToLower(line);
	return line;
}

int analyse(char *line) {
	int keywordsFound = 0, num = sizeof(keys)/sizeof(keys[0]), i;

	for (i = 0; i < num; i++) {
		keywordsFound += substringSearch(line, keys[i]);
	}
	
	return (keywordsFound > 0);
}

void readAvailableData() {
	char *currLine = NULL;
	while (1) {
		currLine = readLine();
		if (currLine == NULL)
			break;

		struct logEntry *currLog = parseEntry(currLine);
		struct logEntryNode *currlogEntryNode = malloc(sizeof(struct logEntryNode));

		currlogEntryNode->curr = currLog;
		currlogEntryNode->next = logEntryList;

		logEntryList = currlogEntryNode;

		if (analyse(currLine)) {
			printf("%s\n", currLine);
			updateNumProcessFail(currLog);
		}
	}
}

int main() {
	logEntryList = malloc(sizeof(struct logEntryNode));
	while(1) {
		openFile();
		readAvailableData();
		closeFile();

		sleep(30);
	}

	return 0;
}
