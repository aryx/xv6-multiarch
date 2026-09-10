// The xv6 user-level API, shared by every port under forks/.
//
// claude: base taken verbatim from forks/arm64, which already declared
// exactly the set every port has in common - 33 system calls and ulib
// entry points shared by all fourteen, plus memcmp/memcpy, which the
// shared lib_core/libc/ulib/ulib.c now defines everywhere even though
// only four ports used to declare them.
//
// A port that has something extra keeps its own user/user.h, which is found
// first (its fork root is earlier on the include path) and which includes
// this file by relative path. Only three do: amd64-jserv adds chmod(),
// arm-pi3 adds time(), and riscv64 adds sys_sbrk/sbrklazy/pause/sync for its
// lazy-allocation work.
struct stat;
struct rtcdate;

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(const char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void fprintf(int, const char*, ...);
void printf(const char*, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);
int memcmp(const void *, const void *, uint);
void *memcpy(void *, const void *, uint);
