#include "arm.h"
#include "memlayout.h"

.text
.code 32

.global _start

_start:
    ;@ claude: park secondary cores immediately, before ANY shared-state
    ;@ setup below - reads MPIDR, and any core whose low affinity bits
    ;@ are nonzero sleeps in a WFE loop instead of falling through. All 4
    ;@ emulated Cortex-A7 cores under QEMU's "-M raspi2b" (and real
    ;@ Cortex-A7 hardware) start executing _start SIMULTANEOUSLY - this
    ;@ codebase had no core-ID check anywhere at all before this fix
    ;@ (confirmed via a repo-wide grep), so all 4 cores would otherwise
    ;@ race to clear the SAME .bss and set SP to the SAME shared
    ;@ "svc_stktop", a genuine multi-core corruption bug, not a
    ;@ single-core one - matches the sibling forks/rpi2 port's own
    ;@ working entry.S pattern for the same real hardware family (real
    ;@ Cortex-A7), see notes_arch_armv7_rpi.txt's own "Gap".
    mrc p15, 0, r0, c0, c0, 5      ;@ MPIDR
    ands r0, r0, #0x03
    wfene
    bne park

    ;@ claude: guarded with a mode check - ELR_hyp/eret are Hyp-mode-only
    ;@ banked registers/instructions, Undefined Instruction from any
    ;@ other mode. QEMU's "-M raspi2b" (and likely real hardware's own
    ;@ boot firmware too) already drops the CPU to SVC mode as part of
    ;@ its own EL3/monitor handoff before jumping here, so unconditionally
    ;@ executing this hyp-exit sequence faulted immediately (confirmed
    ;@ via "-d int,guest_errors": a repeating Undefined Instruction loop
    ;@ right after the firmware handoff - see notes_arch_armv7_rpi.txt).
    ;@ Skip straight to the SVC-mode setup below when not actually in
    ;@ Hyp mode.
    mrs r0,cpsr
    and r1,r0,#0x1F
    cmp r1,#HYP_MODE
    bne 1f
    ;@ return from hypervisor
    bic r0,r0,#0x1F
    orr r0,r0,#0x13
    msr spsr_cxsf,r0
    add r0,pc,#4
    msr ELR_hyp,r0
    eret
1:

    // Setup SVC mode stack
    mov r0, #(SVC_MODE | NO_INT)
    msr cpsr_c, r0
    ldr     sp, =svc_stktop

    // Clear out bss.
    ldr r4, =edata_entry
    ldr r9, =end_entry
    mov r5, #0
    mov r6, #0
    mov r7, #0
    mov r8, #0
    b       2f

1:
    // store multiple at r4.
    stmia r4!, {r5-r8}

    // If we are still below bss_end, loop.
2:
    cmp r4, r9
    blo 1b

    // jump to start()
    b start

    B .

park:
    wfe
    b park

# during startup, kernel stack uses user address, now switch it to kernel addr
.global jump_stack
jump_stack:
    MOV     r0, sp
    ADD     r0, r0, #KERNBASE
    MOV     sp, r0
    MOV     pc, lr