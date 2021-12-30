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

// Enumeration of error types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// Enumeration of lval types
enum { LVAL_NUM, LVAL_ERR };

// Define lval strcut
typedef struct {
    int type;
    long num;
    int err;
} lval;

// Create a number type lval
lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

// Create a error type lval
lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.num = x;
    return v;
}

// Print an lval
void lval_print(lval v) {
    switch (v.type) {
        // If  this a number type lval, print the number
        case LVAL_NUM:
            printf("%li", v.num);
            break;
        case LVAL_ERR:
            // Determine type of error and print it
            if (v.err == LERR_DIV_ZERO) {
                printf("Error: Division By Zero!");
            }
            if (v.err == LERR_BAD_OP) {
                printf("Error: Invalid Operator!");
            }
            if (v.err == LERR_BAD_NUM) {
                printf("Error: Invalid Number!");
            }
            break;
    }
}

// Print an lval followed by a newline
void lval_println(lval v) {
    lval_print(v);
    putchar('\n');
}

// Simple min function; returns the smallest number
long min(long x, long y) {
    return (x < y ? x : y);
}

// Simple max function; returns the largest number
long max(long x, long y) {
    return (x > y ? x : y);
}

// Determine proper operation based on operator string
lval eval_op(lval x, char* op, lval y) {

    // If either value is an error, return it
    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }

    // Perform arithmetic
    if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
    if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
    if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }

    // Check for division by zero
    if (strcmp(op, "/") == 0) { 
        return y.num == 0
            ? lval_err(LERR_DIV_ZERO)
            : lval_num(x.num / y.num); 
    }

    if (strcmp(op, "%") == 0) { return lval_num(x.num % y.num); }
    if (strcmp(op, "^") == 0) { return lval_num(pow(x.num, y.num)); }

    if (strcmp(op, "min") == 0) { return lval_num(min(x.num, y.num)); }
    if (strcmp(op, "max") == 0) { return lval_num(max(x.num, y.num)); }

    return lval_err(LERR_BAD_OP);
}

// Evaluate the given AST
lval eval(mpc_ast_t* t) {

    if (strstr(t->tag, "number")) {
        // Check if there is some error in conversion
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    // The operator is always the second child
    char* op = t->children[1]->contents;

    // Store the third child in 'x'
    lval x = eval(t->children[2]);

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

    puts("Blisp Version 0.0.0.0.4");
    puts("Author: Brendan Castle\n");
    puts("Based on 'Build Your Own Lisp' by Daniel Holden\n");
    puts("Press Ctrl+c to Exit\n");

    while(1) {

        char* input = readline("blisp: ");
        add_history(input);

        // Attempt to parse user input
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Blisp, &r)) {
            lval result = eval(r.output);
            lval_println(result);
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