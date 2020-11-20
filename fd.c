#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>
#include <fnmatch.h>

// TODO
// support for path with spaces  

//for -t option adding character to dest 
void add_activetype(char *dest, int size, char toadd)
{
    int ispresent = 0;
    for (int i = 0; i < size; i++)
    {

        if (dest[i] == toadd)
        {
            ispresent = 1;
            break;
        }
        else
        {
            ispresent = 0;
        }
    }
    if (!ispresent)
    {
        strncat(dest, &toadd, 1);
    }
}

//check if option ins valid 
int is_valid(char input)
{
    char *type = "fdlxesp";
    for (int i = 0; i < 8; i++)
    {
        if (type[i] == input)
        {
            return 1;
        }
    }

    return 0;
}


//check file type 
char get_mode(mode_t mode)
{

    char c = '?';

    if (S_ISDIR(mode))
    {
        c = 'd';
    }
    else if (S_ISREG(mode))
    {
        if (mode & S_IXUSR)
        {
            c = 'x';
        }
        else
        {
            c = 'f';
        }
    }
    else if (S_ISCHR(mode))
    {
        c = 'c';
    }
    else if (S_ISBLK(mode))
    {
        c = 'b';
#ifdef S_ISLNK
    }
    else if (S_ISLNK(mode))
    {
        c = 'l';
#endif
#ifdef S_ISSOCK
    }
    else if (S_ISSOCK(mode))
    {
        c = 's';
#endif
    }
    else if (S_ISFIFO(mode))
    {
        c = 'p';
    }
    return c;
}

//check file type advanced
char type(char *path)
{

    struct stat s;
    int r;
    char c = '?';

    r = lstat(path, &s);
    if (r == -1)
    {
        perror("state or lstate");
        return c;
    }

    c = get_mode(s.st_mode);

    //check for empty file or empty exec file
    if ((c == 'f') & (s.st_size == 0))
    {
        c = 'e';
    }
    else if ((c == 'x') & (s.st_size == 0))
    {
        c = 'z';
    }
    return c;
}
//print if active type valid
void print_path_help(char *path, char *activetype)
{

    char tp = type(path);
    int isfile = 0;
    int isexec = 0;
    int isemp = 0;
    int printed = 0;
    //if option given 
    if (strlen(activetype) >= 1)
    {
        for (size_t i = 0; i < strlen(activetype); i++)
        {
            if (activetype[i] == tp)
            {
                puts(path);
                printed = 1;
                break;
            }
            else if (activetype[i] == 'f')
            {
                isfile = 1;
            }
            else if (activetype[i] == 'x')
            {
                isexec = 1;
            }
            else if (activetype[i] == 'e')
            {
                isemp = 1;
            }
        }
        //if file given print empty , exec, exec empty files also
        if ((!printed) & (isfile) & ((tp == 'e') || (tp == 'z') || (tp == 'x')))
        {
            puts(path);
        } // if exec as option print exec empty also
        else if ((!printed) & isexec & (tp == 'z'))
        {
            puts(path);
        } //is empty given print empty exec also
        else if (((!printed) & isemp & (tp == 'z')))
        {
            puts(path);
        }
    }
    else
    {
        puts(path);
    }
}

int pathprint(char *path, char *pat, char *activetype, int patmatch, int lev)
{
    DIR *d;
    struct dirent *e;

    // printf("DIR DIR: %s", path);
    d = opendir(path);
    if (!d)
    {
        perror("opendir");
        fprintf(stderr, "Dir Path : %s\n", path);
        exit(EXIT_FAILURE);
    }

    // if (chdir(path) == -1)
    // {
    //     perror("chdir");
    //     exit(EXIT_FAILURE);
    // }

    char fullpath[4096];
    fullpath[0] = '\0';
    while (1)
    {
        patmatch = 0; //reset after directory print ends

        e = readdir(d);
        if (!e)
        {
            break;
        }
            //skip tese directories
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
            continue;

        if (lev == 0) //base directory
        {
            if (!fnmatch(pat, e->d_name, 0)) //if pattern match 
            {
                patmatch = 1;
            }
            if (patmatch)
            {
                print_path_help(e->d_name, activetype);
            }

            if (e->d_type == DT_DIR) //if directory recursive and not base
            {
                pathprint(e->d_name, pat, activetype, patmatch, 1);
            }
        }
        else //if not base  generate full path 
        {
            strncpy(fullpath, path, strlen(path));
            fullpath[strlen(path)] = '\0';
            strcat(fullpath, "/");
            strcat(fullpath, e->d_name);

            if (!fnmatch(pat, e->d_name, 0))
            {
                patmatch = 1;
            }
            if (patmatch)
            {
                print_path_help(fullpath, activetype);
            }
            if (e->d_type == DT_DIR)
            {
                pathprint(fullpath, pat, activetype, patmatch, 1);
                fullpath[0] = '\0';

            }
        }
    }
    (void)closedir(d);
    return 0;
}

int main(int argc, char *argv[])
{

    char activetype[10];
    int c;

    while ((c = getopt(argc, argv, "t:")) >= 0)
    {
        switch (c)
        {
        case 't':
            if (is_valid(optarg[0]))
            {
                add_activetype(activetype, 10, optarg[0]);
            }
        }
    }

    char *pat = "*";
    char *defpath = "./";

    if (!(argv[optind] == NULL)) //for pattern
    {
        pat = argv[optind];
    }

     //   printf("%d and %d\n",optind,argc);

    if ((optind < argc-1) & !(argv[argc - 1] == NULL)) //for path
    {
        defpath = argv[argc - 1];
        pathprint(defpath, pat, activetype, 0, 1);
        return 0;
    }
    //if no  pattern and path given
    pathprint(".", pat, activetype, 0, 0);

    return 0;
}
