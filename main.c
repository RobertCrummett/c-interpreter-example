#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define int long long

int token; // current token
int *src, *old_src; // pointer to the source code string
int poolsize;
int line; // line number
int *text, *old_text, *stack; // text segment, dump text segment, and stack
char *data; // data segment
int *pc, *bp, *sp, ax, cycle; // virtual machine registers

//
// Virtual machine instruction set
//
enum { LEA, IMM, JMP, CALL, JZ, JNZ, ENT, ADJ, LEV, LI, LC, SI, SC, PUSH,
	OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
	OPEN, READ, CLOS, PRTF, MALC, MSET, MCMP, EXIT };

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
		// printf("token is: %d\n", token);
		next();
	}
}

int eval(void) { // do nothing yet
	int op, *tmp;
	while (1) {
		op = *pc++;
		if (op == IMM)        ax = *pc++;  // IMM moves next number into AX
		else if (op == LC)    ax = *(char *)ax;  // LC loads AX as a character just before execution
		else if (op == LI)    ax = *(int *)ax;  // LI loads AX as an integer just before execution
		else if (op == SC)    ax = *(char *)*sp++ = ax;  // SC stores character in AX onto top of stack
		else if (op == SI)    *(int *)*sp++ = ax; // SI stores integer in AX onto top of stack
		else if (op == PUSH)  *--sp = ax; // PUSH value in AX onto stack
		else if (op == JMP)   pc = (int *)*pc; // set value of PC to address
						       // NOTE: PC points to the NEXT instruction to be evaluated
		//
		// JZ and JNZ are virtual functions to implement conditional logic (ie, `if` statements)
		//
		else if (op == JZ)    pc = ax ? pc + 1 : (int *)*pc;
		else if (op == JNZ)   pc = ax ? (int *)*pc : pc + 1;
		//
		// Frames muthafucker!
		//
		else if (op == CALL)  {
			*--sp = (int)(pc + 1);
			pc = (int *)*pc;
		} // else if (op == RET) pc = (int *)*sp++;
		else if (op == ENT)   {
			// ENTer the new calling frame
			*--sp = (int)bp;
			bp = sp;
			sp = sp - *pc++;
		} else if (op == ADJ) sp = sp + *pc++; // ADJustment to the stack --- remove arguments from the frame
		else if (op == LEV) { // LEaVe the current frame gracefully
			sp = bp;
			bp = (int *)*sp++;
			pc = (int *)*sp++;
		} else if (op == LEA) ax = (int)(bp + *pc++);
		//
		// Maths!
		//
		else if (op == OR)  ax = *sp++ | ax;
		else if (op == XOR) ax = *sp++ ^ ax;
		else if (op == AND) ax = *sp++ & ax;
		else if (op == EQ)  ax = *sp++ == ax;
		else if (op == NE)  ax = *sp++ != ax;
		else if (op == LT)  ax = *sp++ < ax;
		else if (op == LE)  ax = *sp++ <= ax;
		else if (op == GT)  ax = *sp++ >  ax;
		else if (op == GE)  ax = *sp++ >= ax;
		else if (op == SHL) ax = *sp++ << ax;
		else if (op == SHR) ax = *sp++ >> ax;
		else if (op == ADD) ax = *sp++ + ax;
		else if (op == SUB) ax = *sp++ - ax;
		else if (op == MUL) ax = *sp++ * ax;
		else if (op == DIV) ax = *sp++ / ax;
		else if (op == MOD) ax = *sp++ % ax;
		//
		// Harder functions....
		//
		else if (op == EXIT) { printf("exit(%lld)", *sp); return *sp;}
		else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
		else if (op == CLOS) { ax = close(*sp);}
		else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp); }
		else if (op == PRTF) { tmp = sp + pc[1]; ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
		else if (op == MALC) { ax = (int)malloc(*sp);}
		else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp);}
		else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp);}
		else {
			fprintf(stderr, "ERROR unknown instruction: %lld\n", op);
			return -1;
		}
	}
}

int main(void) {
	line = 1;
	//
	// Open the file path and read its entire contents into a single string.
	//
	const char *path = "main.c";
	FILE *file = fopen(path, "r");
	if (ferror(file)) {
		fprintf(stderr, "ERROR while opening %s: %s\n", path, strerror(errno));
		return 1;
	}
	if (fseek(file, 0, SEEK_END)) { // Find the end of the file
		fprintf(stderr, "ERROR while seeking end of %s: %s\n", path, strerror(errno));
		fclose(file);
		return 1;
	}
	poolsize = ftell(file); // Report the length of the file in bytes
	if (poolsize == -1) {
		fprintf(stderr, "ERROR determining size (in bytes) of %s: %s\n", path, strerror(errno));
		fclose(file);
		return 1;
	}
	if (fseek(file, 0, SEEK_SET)) { // Rewind the file to the beginning
		fprintf(stderr, "ERROR while rewinding %s: %s\n", path, strerror(errno));
		fclose(file);
		return 1;
	}
	src = malloc(poolsize); // Allocate space for the file in memory
	if (src == NULL) {
		fprintf(stderr, "ERROR allocating memory (%lld bytes) for %s: %s\n", poolsize, path, strerror(errno));
		fclose(file);
		return 1;
	}
	if (fread(src, poolsize, 1, file)) { // Read the file contents into memory
		fprintf(stderr, "ERROR reading %s into memory: %s\n", path, strerror(errno));
		fclose(file); free(data);
		return 1;
	};
	fclose(file);
	old_src = src;
	//
	// Now allocate memory for the text segment, data segment, and the
	// stack of our program. Also zero initialize them. There is no heap
	// or bss supported by this virtual machine, so we do not allocate
	// them directly. bss and data sections are implicitly merged. And
	// heap we just do not care about, lol. That is fine though becase
	// all I am here to do is write an interpreter
	//
	// Note that we will only be storing string literals in the data
	// section. This is for convenience.
	//
	// Note that the text and stack are implemented in terms of int's,
	// not unsigned types. Although unsigned is preferable, we want our
	// compiler to bootstrap itself eventually, so we will use int instead
	// of unsigned data type. I am not sure why we could not use unsigned
	// anyway??? But this may be for convenience later on.
	//
	text = old_text = calloc(poolsize, 1);
	if (text == NULL) {
		fprintf(stderr, "ERROR failed to allocate text segment (%lld bytes): %s\n", poolsize, strerror(errno));
		free(src);
		return 1;
	}
	data = calloc(poolsize, 1);
	if (data == NULL) {
		fprintf(stderr, "ERROR failed to allocate data segment (%lld bytes): %s\n", poolsize, strerror(errno));
		free(src); free(text);
		return 1;
	}
	stack = calloc(poolsize, 1);
	if (stack == NULL) {
		fprintf(stderr, "ERROR failed to allocate stack segment (%lld bytes): %s\n", poolsize, strerror(errno));
		free(src); free(text); free(data);
		return 1;
	}
	//
	// Registers are used to store the running state of computers.
	// Our virtual machine uses 4:
	//   PC - program counter, stores the next instruction
	//   SP - stack pointer, points at the top of the stack
	//        NOTE: the stack grows from high address to low address
	//   BP - base pointer, points to some elements on the stack & used for function calls
	//   AX - general register used to store result of instruction
	//
	bp = sp = (int *)((int)stack + poolsize);
	ax = 0;
	//
	// Test the assembly code by calculating `10 + 20`
	//
	int i = 0;
	text[i++] = IMM;
	text[i++] = 10;
	text[i++] = PUSH;
	text[i++] = IMM;
	text[i++] = 20;
	text[i++] = ADD;
	text[i++] = PUSH;
	text[i++] = EXIT;
	pc = text;

	program();
	return eval();
}
