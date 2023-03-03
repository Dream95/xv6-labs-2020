#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void loop(int fd[]);

int main(int argc, char *argv[])
{
    int fd[2];
    pipe(fd);

    for (int i = 2; i <= 35; i++)
    {
        write(fd[1], &i, 4);
    }
    close(fd[1]);
    loop( fd);
    exit(0);
}

void loop(int fd[])
{
    //

    int first;
    if (read(fd[0], &first, 4) > 0)
    {
        printf("prime %d\n", first);
    }
    else
    {

        return;
    }
    int fds[2];
    pipe(fds);
    int num;
    while (read(fd[0], &num, 4))
    {
        if (num % first != 0)
        {
            write(fds[1], &num, 4);
        }
    }
    close(fds[1]);

    if (fork() == 0)
    {
        // child
        loop(fds);
    }else{
        wait(0);
    }
   
}
