#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define STDIN 1

int getfline(int fd)
{
    int i, cc;
    char c;
    int n = 0;
    for (i = 0; i + 1 < 1000;)
    {
        cc = read(0, &c, 1);
        if (cc < 1)
            break;
        if (c == '\n' || c == '\r')
            ++n;
    }
    return n;
}

int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        fprintf(2, "xagrs error\n");
    }

    int n = getfline(STDIN);

   
    for (int i = 0; i < n; i++)
    {
        if (fork() == 0)
        {
            char *args[argc-1];
            memset(args, 0, sizeof(args));
            for (int i = 1; i < argc; i++)
            {
                args[i-1] = argv[i];
            }
            
            exec(argv[1], argv);

            exit(0);
        }
    }
    for (int i = 0; i < n; i++)
    {
        wait(0);
    }

    exit(0);
}