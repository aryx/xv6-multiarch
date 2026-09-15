#ifndef INTERFACE_SYSCALL_H
#define INTERFACE_SYSCALL_H

// claude: syscall.c (the dispatch mechanism - not the individual
// syscall handlers, which are sysfile.c/sysproc.c) has no arch_*.h
// family of its own, but every fork's own implementation converged
// independently on the same function names for the same roles. Each
// fork's own defs.h #includes this file (after its own <arch>_defs.h)
// so the compiler checks the real definition against the declaration
// here, AND every other file that calls these (sysfile.c, sysproc.c,
// ...) sees the same declaration via the usual defs.h chain - see
// kernel/processes/interface_proc.h for the arch_proc.h version of
// the same technique.
//
// arm has a real syscall.c matching the legacy family exactly (it
// only lacks trap.c's own unified trap() - see interface_trap.h -
// dispatching through swi_handler() directly instead).

// int argint(int n, int *ip)
//   Fetch the n'th syscall argument as a plain int. Identical name on
//   all 14 forks with a syscall.c; riscv64 alone returns void (the
//   return value is never checked by any of its own callers either).
#ifdef RISCV64_VOID_ARGS_ABI
void argint(int n, int *ip);
#else
int argint(int n, int *ip);
#endif

// void syscall(...)
//   The dispatcher itself: read the syscall number out of the current
//   trapframe, look it up in a table of function pointers, call it,
//   and store the return value back into the trapframe. amd64 alone
//   takes an explicit struct sysframe *sf argument instead of reading
//   the current process's trapframe implicitly.
#ifdef AMD64_SYSFRAME_ABI
void syscall(struct sysframe *sf);
#else
void syscall(void);
#endif

// --- legacy family (amd64, amd64-jserv, arm, arm-pi1, arm-pi1-bis,
//     arm-pi2, arm-pi3, i386, mips) ---
//
// int argptr(int n, char **pp, int size)
//   Fetch the n'th syscall argument as a pointer, validate that the
//   `size`-byte range starting there lies entirely within the calling
//   process's own address space, and store the (still user-space!)
//   pointer itself into *pp - no copy. Superseded by argaddr() in the
//   modern family below.
int argptr(int n, char **pp, int size);

// int fetchint(uintp addr, int *ip)
//   Read one int from user address `addr` into *ip, after validating
//   the address.
int fetchint(uintp addr, int *ip);

#ifdef LEGACY
// int argstr(int n, char **pp)
// int fetchstr(uintp addr, char **pp)
//   argstr() reads the n'th argument as a user address and hands it
//   to fetchstr(), which validates it points at a NUL-terminated
//   string within the process's own memory and stores a pointer to it
//   (again, no copy) into *pp.
int argstr(int n, char **pp);
int fetchstr(uintp addr, char **pp);
#else
// --- modern family (arm64, arm64-pi4, loongarch, riscv32, riscv64) ---
//
// int argstr(int n, char *buf, int max)
// int fetchstr(uintp addr, char *buf, int max)
//   Same two names as the legacy family above, but a real, different
//   signature and behavior: copies up to `max` bytes of the string
//   INTO a caller-provided kernel buffer (via arch_copyin under the
//   hood) rather than handing back a raw user-space pointer.
int argstr(int n, char *buf, int max);
int fetchstr(uintp addr, char *buf, int max);

// int argaddr(int n, uintp *ip)
//   Fetch the n'th syscall argument as a raw address, no validation -
//   the legacy family's argptr() split into "get the address" (this)
//   and "validate/copy it" (arch_copyin/arch_copyout,
//   kernel/memory/interface_vm.h). riscv64 alone returns void, same
//   as its own argint() above.
#ifdef RISCV64_VOID_ARGS_ABI
void argaddr(int n, uintp *ip);
#else
int argaddr(int n, uintp *ip);
#endif

// int fetchaddr(uintp addr, uintp *ip)
//   Read one address-sized word from user address `addr` into *ip,
//   after validating it - fetchint()'s pointer-width counterpart.
int fetchaddr(uintp addr, uintp *ip);
#endif

// int arguintp(int n, uintp *ip)
//   amd64-jserv's own thin wrapper around argint()/argaddr() (this
//   fork predates the modern family's own argaddr() split, so it grew
//   its own pointer-sized-argument fetcher independently). No other
//   fork defines this name, so declaring it unconditionally is safe -
//   an unused extern prototype is not an error.
int arguintp(int n, uintp *ip);

#endif /* INTERFACE_SYSCALL_H */
