#ifndef INTERFACE_SYSCALL_H
#define INTERFACE_SYSCALL_H

// claude: documentation only - never #included by any build. Same
// kind as kernel/console/interface_console.h and
// kernel/interrupts/interface_trap.h: syscall.c (the dispatch
// mechanism - not the individual syscall handlers, which are
// sysfile.c/sysproc.c in kernel/files//kernel/processes/, the user's
// own correction on this category's real scope) has no arch_*.h
// family of its own, but every fork's own implementation converged
// independently on the same function names for the same roles - the
// richest example of this found so far in this tree, split along the
// usual legacy/modern boundary for every function that has more than
// one shape.
//
// arm is excluded (no syscall.c at all, no syscall mechanism in this
// port the way the other 13 share it).

// void syscall(void)
//   The dispatcher itself: read the syscall number out of the current
//   trapframe, look it up in a table of function pointers, call it,
//   and store the return value back into the trapframe. Called from
//   trap()/usertrap() (kernel/interrupts/interface_trap.h). Same name
//   on 13 of 13 forks with a syscall.c; amd64's own copy takes an
//   explicit struct sysframe *sf argument instead of reading the
//   current process's trapframe implicitly - a real 1-fork signature
//   exception.
void syscall(void);

// int argint(int n, int *ip)
//   Fetch the n'th syscall argument as a plain int. Identical name
//   and signature on all 13 forks - the single most universal
//   function in this whole file.
int argint(int n, int *ip);

// --- legacy family (8 forks: amd64-jserv, amd64, arm-pi1, arm-pi1-bis,
//     arm-pi2, arm-pi3, i386, mips) ---
//
// int argptr(int n, char **pp, int size)
//   Fetch the n'th syscall argument as a pointer, validate that the
//   `size`-byte range starting there lies entirely within the calling
//   process's own address space, and store the (still user-space!)
//   pointer itself into *pp - no copy. Superseded by argaddr() in the
//   modern family below, which separates "get me this address" from
//   "and check this many bytes of it" into two steps.
int argptr(int n, char **pp, int size);

// int argstr(int n, char **pp)
// int fetchstr(uint addr, char **pp)
//   argstr() reads the n'th argument as a user address and hands it
//   to fetchstr(), which validates it points at a NUL-terminated
//   string within the process's own memory and stores a pointer to it
//   (again, no copy - still user-space memory) into *pp.
int argstr(int n, char **pp);
int fetchstr(uint addr, char **pp);

// int fetchint(addr_t addr, int *ip)
//   Read one int from user address `addr` into *ip, after validating
//   the address. Same role and shape everywhere; `addr`'s own spelling
//   varies by fork (uint, uint64 or uintp - each fork's own usual
//   address-width type, not a functional difference). Only the
//   caller's own way of computing `addr` differs (amd64-jserv's own
//   argint() passes proc->tf->esp + 4 + 4*n directly rather than going
//   through a separate helper) - not a different fetchint() shape.
int fetchint(uintp addr, int *ip);

// --- modern family (5 forks: arm64, arm64-pi4, loongarch, riscv32,
//     riscv64) ---
//
// int argaddr(int n, uint64 *ip)
//   Fetch the n'th syscall argument as a raw address, no validation -
//   the legacy family's argptr() split into "get the address" (this)
//   and "validate/copy it" (arch_copyin/arch_copyout,
//   kernel/memory/interface_vm.h), so callers that need to read a
//   whole buffer go through arch_copyin instead of a single
//   size-checked argptr() call.
int argaddr(int n, uintp *ip);

// int argstr(int n, char *buf, int max)
// int fetchstr(uint64 addr, char *buf, int max)
//   Same two names as the legacy family above, but a real, different
//   signature and behavior: copies up to `max` bytes of the string
//   INTO a caller-provided kernel buffer (via arch_copyin under the
//   hood) rather than handing back a raw user-space pointer. Not
//   interchangeable with the legacy family's own (int, char**) pair -
//   documented as two variants, not one, same as
//   kernel/console/interface_console.h's own consoleread/consolewrite.
#if 0
int argstr(int n, char *buf, int max);
int fetchstr(uintp addr, char *buf, int max);
#endif

// int fetchaddr(uint64 addr, uint64 *ip)
//   Read one address-sized word from user address `addr` into *ip,
//   after validating it (fetchint()'s own modern-family, pointer-width
//   counterpart - not present on the legacy family, which has no
//   equivalent split between "int-sized" and "pointer-sized" fetches).
int fetchaddr(uintp addr, uintp *ip);

// --- 1-fork exception, not part of either family above ---
//
// int arguintp(int n, uintp *ip)
//   amd64-jserv's own thin wrapper around argint()/argaddr() (this
//   fork predates the modern family's own argaddr() split, so it grew
//   its own pointer-sized-argument fetcher independently, under the
//   uintp name this fork already uses elsewhere - see
//   docs/claude_notes/plan_factorization.md's own arch_copyin/
//   arch_copyout entry for the full story).
int arguintp(int n, uintp *ip);

#endif /* INTERFACE_SYSCALL_H */
