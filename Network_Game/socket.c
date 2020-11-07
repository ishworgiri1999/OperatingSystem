//igiri@jacobs-university.de


#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>    //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <time.h>     /* time */
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#include "tcp.h"

// ALL TCP_FUNCTIONS 
// Reference: https://cnds.jacobs-university.de/courses/os-2020/src/socket/

typedef struct client
{
    int cfid;
    int score;
    int tries;
    char *currfortune;
    char *word;
    struct client *next;
} Client;

char *readall(int fd)
{
    int buffsize = 256;
    char *buffer = (char *)malloc(buffsize);

    if (!buffer)
    {
        perror("Malloc Failed");
        exit(1);
    }

    int len;
    if ((len = tcp_read(fd, buffer, buffsize)) < 0)
    {
        perror("Read failed readall()");
    }
    buffer[len] = '\0';

    return buffer;
}

//return file descriptor of pipe read 
int fetch()
{

    int pif[2];
    pid_t pid;
    if (pipe(pif) == -1)
    {
        perror("Pipe Creation Failed");
        exit(2);
    }

    pid = fork();
    if (pid < 0)
    {
        perror("Forking Failed");
        exit(3);
    }
    else if (pid == 0)
    { //child
        close(pif[0]);

        //change stdout to fd = pipe
        if (dup2(pif[1], STDOUT_FILENO) == -1)
        {
            perror("dup failed");
            exit(4);
        };

        close(pif[1]);

        char *command = "fortune";
        char *argument_list[] = {"fortune", "-n",
                                 "32", "-s", NULL};

        int status_code = execvp(command, argument_list);
        if (status_code == -1)
        {
            printf("Process did not terminate correctly\n");
            exit(1);
        }
        fflush(stdout);

        close(STDOUT_FILENO);
        exit(EXIT_SUCCESS);
    }
    else
    {
        while (wait(NULL) != pid) //wait for child
        {
            continue;
        }

        close(pif[1]);
        //no write

        //return pipe
        return pif[0];
    }
}

// returns number of words in str
unsigned countWords(char *str)
{ //reference : https://www.geeksforgeeks.org/count-words-in-a-given-string/
    int OUT = 0;
    int IN = 1;
    int state = OUT;
    unsigned wc = 0; // word count

    // Scan all characters one by one
    while (*str)
    {
        // If next character is a separator, set the
        // state as OUT
        if (*str == ' ' || *str == '\n' || *str == '\t')
            state = OUT;

        // If next character is not a word separator and
        // state is OUT, then set the state as IN and
        // increment word count
        else if (state == OUT)
        {
            state = IN;
            ++wc;
        }

        // Move to next character
        ++str;
    }

    return wc;
}


//gets random word to hide from string 
char *getword(char *line)
{
    char *linecopy = (char *)malloc(strlen(line) + 1);

    if (linecopy == NULL)
    {
        perror("malloc getword failed");
        exit(5);
    }
    strncpy(linecopy, line, strlen(line) + 1);

    int n_words = countWords(line);

    srand(time(NULL));
    int w_pos = rand() % n_words;

    int i = 0;

    char *word;
    word = strtok(linecopy, " \",.-!:'");
    while (i < w_pos & word != NULL)
    {
        word = strtok(NULL, " \",.-!:'");
        i++;
    }
    char *word_cpy = (char *)malloc(strlen(word) + 1);

    if (word_cpy == NULL)
    {
        perror("malloc getword failed");
        free(linecopy);
        exit(5);
    }
    strncpy(word_cpy, word, strlen(line) + 1);
    free(linecopy);
    return word_cpy;
}

//returns string after hinding word to be guessed
char *put_fortune(char *word, char *line)
{
    char *linecopy = (char *)malloc(strlen(line) + 1);
    if (linecopy == NULL)
    {
        perror("malloc put_fortune failed");
        exit(5);
    }

    strncpy(linecopy, line, strlen(line) + 1);
    char *pos = strstr(linecopy, word);
    memset(pos, '_', strlen(word));

    return linecopy;
}

int main(int argc, char const *argv[])
{

    if (argc < 2)
    {
        printf("Usage portno");
        exit(1);
    }

    Client *head = NULL;

    int fdc;
    int server_socket = tcp_listen("127.0.0.1", argv[1]);

    fd_set current, ready;

    FD_ZERO(&current);
    FD_SET(server_socket, &current);
    while (1)
    {
        ready = current; //destructive in nature
        if (select(FD_SETSIZE, &ready, NULL, NULL, NULL) < 0)
        {
            perror("select error");
            exit(5);
        }

        int fortune_pipe;     //read pipe from fetch()
        char cl_return[256];  //return data from client
        char *word;           //word guess
        char *final, *tosend; //modified string to send

        for (int i = 0; i < FD_SETSIZE; i++) //checking all fds
        {
            if (FD_ISSET(i, &ready))
            {
                if (i == server_socket) //if socket accept connection
                {

                    fdc = accept(server_socket, NULL, NULL);
                    FD_SET(fdc, &current);

                    //create client node and initialize it
                    Client *newc = (Client *)malloc(sizeof(Client));
                    newc->next = head;
                    head = newc;
                    newc->cfid = fdc;
                    newc->score = 0;
                    newc->tries = 1;

                    tcp_write(fdc, "M: Guess the missing ____!\nM: Send your guess in the form 'R: word\\r\\n'. \n", 75);
                    fortune_pipe = fetch();
                    final = readall(fortune_pipe);

                    close(fortune_pipe);

                    // print to server for debug or guess answer and add to client details
                    word = getword(final);
                    printf("%s // %s\n", final, word);
                    tosend = put_fortune(word, final);
                    newc->currfortune = tosend;
                    newc->word = word;
                    free(final);
                    //send question
                    tcp_write(fdc, "C: ", 4);
                    tcp_write(fdc, tosend, strlen(newc->currfortune));
                    free(newc->currfortune);
                }
                else //if ready to read from client
                {

                    Client *cursor = head; //find which client is ready

                    while ((cursor->next != NULL) & (cursor->cfid != i))
                    {
                        cursor = cursor->next;
                    }

                    int len_c = read(i, cl_return, 256);
                    //get data
                    cl_return[len_c - 1] = '\0';

                    char *q_pos = strstr(cl_return, "Q:");

                    if (q_pos) //for exit
                    {
                        char byemsg[256];
                        sprintf(byemsg, "M: You mastered %d/%d challenges. Good bye!\n", cursor->score, cursor->tries);
                        tcp_write(i, byemsg, strlen(byemsg));
                        FD_CLR(i, &current);
                        close(i);
                    }

                    char *pos = strstr(cl_return, "R: "); //for result

                    if (!pos)//#if invalid input format
                    {
                        tcp_write(i, "F: bad input try again\n", 24); 
                        continue;
                    }

                    char *data = (char *)malloc(len_c);
                    strncpy(data, pos + 3, len_c);
                    // free(cl_return);

                    if (strcmp(cursor->word, data) == 0)
                    {
                        cursor->score++;
                        tcp_write(i, "O: Congratulation - challenge passed!\n", 39);
                    }
                    else
                    {
                        tcp_write(i, "F: Wrong guess - expected: ", 28);
                        tcp_write(i, cursor->word, strlen(cursor->word));
                        tcp_write(i, "\n", 2);
                    }
                    free(cursor->word);

                    fortune_pipe = fetch(); //get new fortune
                    final = readall(fortune_pipe);

                    close(fortune_pipe);
                    word = getword(final);
                    printf("%s // %s\n", final, word);
                    tosend = put_fortune(word, final);
                    free(final);

                    cursor->currfortune = tosend;
                    cursor->word = word;
                    cursor->tries++;
                    tcp_write(fdc, "C: ", 4);
                    tcp_write(i, tosend, strlen(cursor->currfortune));
                    free(cursor->currfortune);

                    // free(result);
                }
            }
        }

        // char *result;
    }

    return 0;
}
