#include "quiz.h"



// Function to parse Json only for this project  api
struct json *parse(char *jsonstring)
{
    int len = strlen(jsonstring);
    char *pos1 = strstr(jsonstring, "\"text\":");
    char *pos2 = strstr(jsonstring, "\"number\":");
    char *pos3 = strstr(jsonstring, "\"found\":");

    if (pos1 == NULL || pos2 == NULL || pos3 == NULL)
    {
        return NULL;
    }

    pos1 = pos1 + 9; // "text": "  len =9 move 9 byte
    pos2 = pos2 - 4; // ending of text
    int len1 = pos2 - pos1;
    char *text = (char *)malloc(len1 + 1);
    memcpy(text, pos1, len1);
    text[len1 + 1] = '\0';
    pos2 = pos2 + 4 + 10; //getting number
    pos3 = pos3 - 3;
    int len2 = pos3 - pos2; //length of text
    char *numberstr = (char *)malloc(len2 + 1);
    memcpy(numberstr, pos2, len2);
    numberstr[len2 + 1] = '\0'; //ending text

    int number = atoi(numberstr); //number to int

    free(numberstr);
    struct json *parsed = (struct json *)malloc(sizeof(struct json) * 1);
    parsed->number = number;
    parsed->text = text;

    return parsed;
}

char *readall(int fd)
{

    char *buffer = (char *)malloc(BUFFSIZE);

    if (!buffer)
    {
        perror("Malloc Failed");
        exit(1);
    }

    int len;
    if ((len = read(fd, buffer, BUFFSIZE)) < 0)
    {
        perror("Read failed readall()");
    }
    buffer[len] = '\0';

    return buffer;
}


//Fetch and returns in json format [string]

char *fetch()
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
        dup2(pif[1], STDOUT_FILENO);

        close(pif[1]);

        char *command = "curl";
        char *argument_list[] = {"curl", "-s",
                                 "http://numbersapi.com/random/math?min=1&max=100&fragment&json", NULL};

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
        char *buffer;

        close(pif[1]);
        //no write

        buffer = readall(pif[0]); //get from pipe

        close(pif[0]);
        return buffer;
    }
}


//Functionn to play each game
unsigned play(unsigned n, unsigned score, char *text, long answer)
{
    int point = 8; //starting point 8
    long input;

    printf("Q%d: What is %s?\n", n, text);
    while (point) //if point for this try 0 -> exit
    {

        printf("%d pt> ", point);

        char buffer[50];
        if (fgets(buffer, sizeof buffer, stdin) == NULL)
        {
            printf("\n\nYour total score is %d/%d\n\n", score, (n - 1) * 8);
            free(text);
            exit(100);
        }

        if ((input = atol(buffer)) == 0L)
        {
            printf("invalid input\n");
            continue;
        }

        if (input > answer)
        {
            point = point / 2;
            if (point == 0)
            {
                printf("Too Big, the correct answer was %ld.\n", answer);
            }
            else
            {
                printf("Too Big, try again.\n");
            }

            continue;
        }
        else if (input < answer)
        {
            point = point / 2;

            if (point == 0)
            {
                printf("Too Small, the correct answer was %ld.\n", answer);
            }
            else
            {

                printf("Too Small, try again.\n");
            }
            continue;
        }
        else
        {
            score = score + point;
            printf("Congratulation, your answer %ld is correct.", input);
            break;
        }
    }
    printf("Your total score is %d/%d\n\n", score, n * 8);

    return score;
}
