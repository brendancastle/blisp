#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>

// If we are compiling on Windows, compile these functions
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

// Fake readline function
char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy;
}

// Fake add history function
void add_history(char* unused) {}

// Otherwise, include the editline headers
#else
#include <editline/readline.h>
#endif

// Simple min function; returns the smallest number
long min(long x, long y) {
    return (x < y ? x : y);
}

// Simple max function; returns the largest number
long max(long x, long y) {
    return (x > y ? x : y);
}

// Determine proper operation based on operator string
long eval_op(long x, char* op, long y) {
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    if (strcmp(op, "%") == 0) { return x % y; }
    if (strcmp(op, "^") == 0) { return pow(x, y); }
    if (strcmp(op, "min") == 0) { return min(x, y); }
    if (strcmp(op, "max") == 0) { return max(x, y); }

    return 0;
}

// Evaluate the given AST
long eval(mpc_ast_t* t) {

    // If tagged as number, return it directly
    if (strstr(t->tag, "number")) {
        return atoi(t->contents);
    }

    // The operator is always the second child
    char* op = t->children[1]->contents;

    // Store the third child in 'x'
    long x = eval(t->children[2]);

    // Iterate the remaining children
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(int argc, char** argv) {

    // Create parsers
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Operator  = mpc_new("operator");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Blisp     = mpc_new("blisp");

    // Define parsers with the following language
    mpca_lang(MPCA_LANG_DEFAULT,
      "                                                         \
        number      : /-?[0-9]+/ ;                              \
        operator    : '+' | '-' | '*' | '/' | '%' | '^' |       \
                      \"min\" | \"max\" ;                       \
        expr        : <number> | '(' <operator> <expr>+ ')' ;   \
        blisp       : /^/ <operator> <expr>+ /$/ ;              \
      ",
      Number, Operator, Expr, Blisp);

    puts("Blisp Version 0.0.0.0.2");
    puts("Author: Brendan Castle\n");
    puts("Based on 'Build Your Own Lisp' by Daniel Holden\n");
    puts("Press Ctrl+c to Exit\n");

    while(1) {

        char* input = readline("blisp: ");
        add_history(input);

        // Attempt to parse user input
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Blisp, &r)) {
            long result = eval(r.output);
            printf("%li\n", result);
            mpc_ast_delete(r.output);
        } 
        else {
            // Otherwise, print the error
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    // Undefine and delete the parsers
    mpc_cleanup(4, Number, Operator, Expr, Blisp);

    return 0;
}