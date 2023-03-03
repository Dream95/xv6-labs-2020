#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char **argv)
{

    int ptc[2], ctp[2];
    pipe(ptc);
    pipe(ctp);
    int pid = fork();
    char buf[1];
    if (pid == 0)
    {
        // child
        if (read(ptc[0], buf, 1))
        {
            printf("%d: received ping\n", getpid());
            write(ctp[1],"b",1);
        }
    }
    else
    {
      
        write(ptc[1], "x", 1);

        if (read(ctp[0], buf, 1))
        
            printf("%d: received pong\n", getpid());
        
    }
    exit(0);
}
