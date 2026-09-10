// Test that fork fails gracefully.
// Tiny executable so that the limit can be filling the proc table.

#include "types.h"
#include "stat.h"
#include "user.h"

#define N  1000

// claude: renamed printf -> print, matching mit-pdos/xv6-riscv. forktest
// deliberately links without ulib's printf.o (it must stay tiny enough to
// fill the proc table), so it supplies its own; under the 2019 API that
// local definition would otherwise clash with user.h's printf declaration.
void
print(const char *s)
{
  write(1, s, strlen(s));
}

void
forktest(void)
{
  int n, pid;

  print("fork test\n");

  for(n=0; n<N; n++){
    pid = fork();
    if(pid < 0)
      break;
    if(pid == 0)
      exit(0);
  }

  if(n == N){
    print("fork claimed to work N times!\n");
    exit(0);
  }

  for(; n > 0; n--){
    if(wait(0) < 0){
      print("wait stopped early\n");
      exit(0);
    }
  }

  if(wait(0) != -1){
    print("wait got too many\n");
    exit(0);
  }

  print("fork test OK\n");
}

int
main(void)
{
  forktest();
  exit(0);
}
