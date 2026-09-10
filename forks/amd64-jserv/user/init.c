// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"
#include "file.h"

char *argv[] = { "sh", 0 };

int
main(void)
{
  int pid, wpid;


  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  mknod("cpuid", CPUID, 1);

  for(;;){
    printf("init: starting sh\n");
    pid = fork();
    if(pid < 0){
      printf("init: fork failed\n");
      exit(0);
    }
    if(pid == 0){
      exec("sh", argv);
      printf("init: exec sh failed\n");
      exit(0);
    }
    while((wpid=wait(0)) >= 0 && wpid != pid)
      printf("zombie!\n");
  }
}
