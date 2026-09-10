// init: the first user program. Creates /console, then keeps a shell running.
//
// claude: base taken from forks/riscv64, the most complete of the fourteen.
// The older 37-line variant that nine ports carried differs in two ways that
// are worth keeping the newer behaviour for: it exits 0 on fork/exec failure
// rather than 1, and it prints "zombie!" for every parentless process it
// reaps, which is noise rather than information. This version distinguishes
// the shell exiting - restart it - from an orphan being reaped - do nothing -
// and reports a genuine wait() error.
//
// mknod("console", CONSOLE, 0) works on every port: CONSOLE is already
// defined as 1 in all fourteen kernels, and the minor number is stored in
// the inode but never dispatched on - devsw is indexed by major alone. The
// nine older ports passed minor 1; nothing read it.
// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

// claude: CONSOLE lives in kernel/file.h in all fourteen ports, but that
// header is not self-contained everywhere - riscv64's needs spinlock.h,
// sleeplock.h and fs.h included first, and several ports have no
// sleeplock.h at all. Pulling that chain into a file shared by fourteen
// header sets is fragile for the sake of one integer, so the console's
// major number is stated here instead. It is 1 in every port; the nine
// older ones passed the literal 1 at the mknod() call site anyway.
#ifndef CONSOLE
#define CONSOLE 1
#endif

char *argv[] = {"sh", 0};

int
main(void)
{
  int pid, wpid;

  if (open("console", O_RDWR) < 0) {
    mknod("console", CONSOLE, 0);
    open("console", O_RDWR);
  }
  dup(0); // stdout
  dup(0); // stderr

  for (;;) {
    printf("init: starting sh\n");
    pid = fork();
    if (pid < 0) {
      printf("init: fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      exec("sh", argv);
      printf("init: exec sh failed\n");
      exit(1);
    }

    for (;;) {
      // this call to wait() returns if the shell exits,
      // or if a parentless process exits.
      wpid = wait((int *)0);
      if (wpid == pid) {
        // the shell exited; restart it.
        break;
      } else if (wpid < 0) {
        printf("init: wait returned an error\n");
        exit(1);
      } else {
        // it was a parentless process; do nothing.
      }
    }
  }
}
