#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>

// igiri@jacobs-university.de

#define N_COINS

pthread_mutex_t g_lock;     //global lock
pthread_mutex_t i_lock;     //iteration lock
pthread_mutex_t c_lock[20]; //each coin lock

char coin[N_COINS] = "OOOOOOOOOOXXXXXXXXXX";

int N = 10000; //default flips
int P = 100;   //default people

void mutex_init() //Initialize All Mutex
{
    (void)pthread_mutex_init(&g_lock, NULL);

    (void)pthread_mutex_init(&i_lock, NULL);

    for (int i = 0; i < 20; i++)
    {
        (void)pthread_mutex_init(&c_lock[i], NULL);
    }
}

void print_coin() //Printing Coins
{
    for (int i = 0; i < 20; i++)
    {
        printf("%c", coin[i]);
    }
}

//Function for Thread using global lock
static void *global_lock(void *attr)
{

    pthread_mutex_lock(&g_lock);
    //critical section accessing coin for N flips
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < 20; j++)
        {
            if (coin[j] == 'O')
            {
                coin[j] = 'X';
            }
            else
            {
                coin[j] = 'O';
            }
        }
    }
    pthread_mutex_unlock(&g_lock);

    return NULL;
}

//Function for Thread using iteration lock
static void *iteration_lock(void *attr)
{
    //int num = *(int *)attr;

    for (int i = 0; i < N; i++)
    {
        pthread_mutex_lock(&i_lock);
        //critical section accessing coin for each iteration
        for (int j = 0; j < 20; j++)
        {
            if (coin[j] == 'O')
            {
                coin[j] = 'X';
            }
            else
            {
                coin[j] = 'O';
            }
        }
        pthread_mutex_unlock(&i_lock);
    }
    return NULL;
};

//Function for Thread using each coin lock
static void *coin_lock(void *attr)
{

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < 20; j++)
        {

            pthread_mutex_lock(&c_lock[j]);

            //critical section accessing coin for each flip
            if (coin[j] == 'O')
            {
                coin[j] = 'X';
            }
            else
            {
                coin[j] = 'O';
            }

            pthread_mutex_unlock(&c_lock[j]);
        }
    }
    return NULL;
}

static void run_threads(int n, void *(*proc)(void *))
{

    int rc;

    pthread_t tid[n]; //threads id
    //before
    for (int i = 0; i < n; i++)
    {
        rc = pthread_create(&tid[i], NULL, proc, NULL);
        if (rc)
        {
            fprintf(stderr, "pthread_create(): %s\n", strerror(rc));
            exit(EXIT_FAILURE);
        }
    }

    //Joining Pthreads
    for (int i = 0; i < n; i++)
    {
        if (tid[i])
        {
            rc = pthread_join(tid[i], NULL);
            if (rc)
            {
                fprintf(stderr, "pthread_join(): %s\n", strerror(rc));
                exit(EXIT_FAILURE);
            }
        }
    }
}

//Function to time each function running on threads.
static double timeit(int n, void *(*proc)(void *))
{
    clock_t t1, t2;
    t1 = clock();
    run_threads(n, proc);
    t2 = clock();
    return ((double)t2 - (double)t1) / CLOCKS_PER_SEC * 1000;
}

//Function to run threads using options
// g- global lock
// i - iteration lock
// c - coin lock
// takes char as input and accepts above options
void run_and_print(char c)
{
    void *fun; //pointing to function
    char l_print[30];
    switch (c)
    {
    case 'g':
        strncpy(l_print, "global lock", 12);
        fun = global_lock;
        break;

    case 'i':
        strncpy(l_print, "iteration lock", 15);
        fun = iteration_lock;
        break;

    case 'c':
        strncpy(l_print, "coin lock", 10);
        fun = coin_lock;
        break;

    default:
        exit(1);
        break;
    }

    printf("coins: ");
    print_coin();
    printf(" (start - %s)\n", l_print);
    double t = timeit(P, fun);
    printf("coins: ");
    print_coin();
    printf(" (end - %s)\n", l_print);
    printf("%d threads x %d flips:    %lf ms\n\n", P, N, t);
}

int main(int argc, char *argv[])
{
    int nflag = 0;
    int pflag = 0;
    int option;

    while ((option = getopt(argc, argv, "n:p:")) != -1)
    {
        switch (option)
        {
        case 'n':
            nflag = 1;
            if (atoi(optarg))
                N = atoi(optarg);
            break;
        case 'p':
            pflag = 1;
            if (atoi(optarg))
                P = atoi(optarg);
            break;
        default:
            return -1;
        }
    }

    mutex_init();
    run_and_print('g'); //using global lock
    run_and_print('i'); //using iteration lock
    run_and_print('c'); //using coin lock
}
