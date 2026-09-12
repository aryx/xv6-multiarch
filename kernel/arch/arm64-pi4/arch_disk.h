#ifndef ARCH_DISK_H
#define ARCH_DISK_H

// claude: this board's disk is a ramdisk, not virtio (no easily-
// accessible real disk controller on real Raspberry Pi 4 hardware) -
// the backend for the shared kernel/bio.c's own disk_rw(b, write)
// interface. A board-specific override of kernel/arch/arm64/
// arch_disk.h (the plain "arm64" QEMU-virt fork's own virtio backend)
// - this fork's Makefile lists this directory's own -I before that
// one's so this file is found first, even though both share
// kernel/arch/arm64 for arch_vm.h/arch_proc.h (genuinely ISA-scoped,
// unlike this file).
#define disk_rw(b, write) ramdiskrw((b), (write))

#endif /* ARCH_DISK_H */
