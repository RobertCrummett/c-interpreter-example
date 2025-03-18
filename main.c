#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void read_entire_file_to_cstr(const char *path, int **data) {
	//
	// Open the file for reading
	//
	FILE *file = fopen(path, "r");
	if (ferror(file)) {
		fprintf(stderr, "ERROR while opening %s: %s\n", path, strerror(errno));
		return;
	}
	//
	// Find the end of the file
	// Non-zero exit code is an error
	//
	if (fseek(file, 0, SEEK_END)) {
		fprintf(stderr, "ERROR while seeking end of %s: %s\n", path, strerror(errno));
		fclose(file);
		return;
	}
	//
	// Report the length of the file in bytes
	// If -1, this means that there was an error
	//
	long length = ftell(file);
	if (length == -1) {
		fprintf(stderr, "ERROR determining size (in bytes) of %s: %s\n", path, strerror(errno));
		fclose(file);
		return;
	}
	//
	// Rewind the file to the beginning
	// Non-zero exit code is an error
	//
	if (fseek(file, 0, SEEK_SET)) {
		fprintf(stderr, "ERROR while rewinding %s: %s\n", path, strerror(errno));
		fclose(file);
		return;
	}
	//
	// Allocate space for the file in memory
	//
	*data = malloc(length * sizeof *data);
	if (*data == NULL) { 
		fprintf(stderr, "ERROR allocating memory (%ld bytes) for %s: %s\n", length, path, strerror(errno));
		fclose(file);
		return;
	}
	//
	// Read the file contents into memory
	// Exit code not equal to 1 means that 1 output was not read into memory.
	//
	if (fread(*data, length, 1, file) != 1) {
		fprintf(stderr, "ERROR reading %s into memory: %s\n", path, strerror(errno));
		fclose(file);
		free(data);
		return;
	};
	fclose(file);
}

int token; // current token
int *src, *old_src; // pointer to the source code string
int line; // line number

void next(void) {
	token = *src++;
	return;
}

void expression(int level) {
	// do nothing
}

void program(void) {
	next(); // get next token
	while (token > 0) {
		printf("token is: %d\n", token);
		next();
	}
}

int eval(void) { // do nothing yet
	return 0;
}

int main(void) {
	line = 1;

	//
	// read entire file
	//
	const char *path = "main.c";
	read_entire_file_to_cstr(path, &src);
	old_src = src;

	program();
	return eval();
}
