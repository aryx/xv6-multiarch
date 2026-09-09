/*
 * claude: armstub64.S - QEMU-only, not part of the real kernel at all.
 * Real Raspberry Pi 3 GPU firmware drops the CPU from its true reset
 * state (EL3, AArch64) down through EL2 before jumping to a 32-bit
 * kernel7.img. QEMU's "-M raspi3b" board-setup ROM (confirmed via
 * gdb-multiarch: this stub's own entry already runs at EL2h AArch64,
 * not EL3 - the board-setup ROM's own "smc 0" call already did the
 * EL3->EL2 half of that drop) does the SAME thing real firmware does,
 * up to EL2 - but real firmware then continues the drop to AArch32
 * before jumping to a 32-bit kernel, and QEMU's raw "-kernel"/
 * "-device loader" loaders don't. This stub supplies exactly that
 * missing EL2->AArch32-EL1 step, then jumps straight into this fork's
 * own 32-bit kernel (loaded separately via a second "-device loader"
 * at ARM_ENTRY - see Makefile).
 */

.section .text
.global _start
_start:
	// claude: release the secondary cores. Only CPU0 ever reaches this
	// stub via "-kernel"'s own direct jump - cores 1-3 stay parked in
	// QEMU's own "raspi_smpboot" ROM (confirmed via the monitor's "info
	// roms"), which polls the REAL hardware spintable release-address
	// slots (physical 0xe0/0xe8/0xf0, for cores 1-3 - core 0 has no
	// slot of its own) - the same protocol real Pi firmware uses, and
	// this kernel's own multiprocessing support (the whole point of
	// forks/arm-pi3/pi_mp) assumes all 4 cores reach ITS entry point
	// together. Writing this stub's own base address there releases
	// them into the SAME stub, so each independently repeats the
	// EL2->AArch32-EL1 drop below and lands at ARM_ENTRY alongside
	// CPU0 - where entry.S's own MPIDR-based primary/secondary gating
	// (the "cmp r0,#0; wfene; bne mp_continue" near its own top) takes
	// over, exactly as it would following real firmware's release.
	//
	// claude: a later session tried gating this block to CPU0 only
	// (cores 1-3 were redundantly re-writing the same three slots and
	// re-issuing their own "dsb sy; sev" once already released - a real
	// hypothesis at the time, since "-d int" tracing had just caught a
	// core taking an "Undefined Instruction" exception "from EL2 to
	// EL2" with ELR=0x300 - raspi_smpboot ROM's own address - after
	// that same core had already dropped to AArch32 EL1). Verified via
	// repeated reboot attempts that the CPU0-only gate did NOT change
	// the failure rate at all - reverted. Left here as a documented
	// dead end: whatever actually causes a core to return to EL2 partway
	// through AArch32 execution, it isn't explained by this stub's own
	// redundant spintable writes - see notes_arch_arm_pi3.txt's own
	// "Open gap" section for where this investigation actually landed.
	ldr	x0, =_start
	ldr	x1, =0xe0
	str	x0, [x1]
	ldr	x1, =0xe8
	str	x0, [x1]
	ldr	x1, =0xf0
	str	x0, [x1]
	dsb	sy
	sev

	// claude: HCR_EL2.RW (bit31) - governs EL1's own execution state
	// once EL2 is implemented (true on this CPU model) - left at its
	// reset value, eret-ing to EL1 landed back in EL2 AArch64 instead
	// (confirmed via gdb: PC ended up at this EL2's own synchronous-
	// exception vector, VBAR_EL2+0x200, meaning the earlier attempt's
	// EL3-register writes themselves UNDEF'd - this code was running
	// at EL2 the whole time, not EL3, so those never should have been
	// EL3 registers to begin with). Zero it explicitly: RW=0 (AArch32).
	mov	x0, #0
	msr	hcr_el2, x0

	// claude: SPSR_EL2: AArch32 SVC mode (M[4:0]=0x13), IRQ/FIQ/Abort
	// masked - this is an eret FROM EL2, so EL2's own SPSR/ELR, not
	// EL3's.
	ldr	x0, =0x1d3
	msr	spsr_el2, x0

	// claude: ELR_EL2: physical address the real (32-bit) kernel is
	// loaded at.
	ldr	x0, =ARM_ENTRY
	msr	elr_el2, x0

	eret
