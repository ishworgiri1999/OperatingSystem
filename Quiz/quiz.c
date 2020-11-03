#include"quiz.h"

int main(int argc, char *argv[])
{

    int errorcount = 0; // if cannot get json 5 times then exit
    struct json *output;

    int score = 0;
    int n = 1;
    while (1)
    {

        char *buffer = fetch();
        if (!buffer)
        {
            continue;
        }

        output = parse(buffer);
        free(buffer);
        if (output == NULL)
        {
            errorcount = errorcount + 1;
            if (errorcount > 5)
            {
                perror("error parsing 5 times in a row");
                exit(2);
            }

            continue;
        }
        else
        {
            errorcount = 0; //reset again
            score = play(n, score, output->text, output->number);
            free(output->text);
            free(output);

            n = n + 1;
        }
        fflush(stdout);
    }
    
    printf("Your total score is %d/%d\n\n", score, n * 8);

    return EXIT_SUCCESS;
}