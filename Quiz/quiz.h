#ifndef QUIZ_H
#define QUIZ_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define BUFFSIZE 4096

//Struct for result of json parsing
struct json
{
    char *text;
    int number;
};

//All functions for this quiz game

struct json *parse(char *jsonstring);
char *readall(int fd);
char *fetch();
unsigned play(unsigned n, unsigned score, char *text, long answer);

#endif
