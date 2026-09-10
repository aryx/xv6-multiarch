#include "types.h"
#include "stat.h"
#include "user.h"


int main(int argc, char *argv[]) {
  int stime = time();
  printf("time is %d\n", stime);
  stime = time();
  printf("time is %d\n", stime);
  while (time() - 2000000 < stime) {

  }
  printf("time is %d\n", time());
  printf("two seconds should have passed\n");
  exit(0);
}
