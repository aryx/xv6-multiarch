#ifndef ARCH_DISK_H
#define ARCH_DISK_H

// claude: this port's disk is virtio - a plain pass-through backend
// for the shared kernel/devices/storage/bio.c's own arch_disk_rw()
// interface. arm64-pi4 (real Raspberry Pi 4 hardware, no virtio
// device) shares this same kernel/devices/storage/arm64 directory for
// arch_vm.h/arch_proc.h - same ISA - but needs a different
// arch_disk_rw() backend (a ramdisk), so it gets its own
// kernel/devices/storage/arm64-pi4/arch_disk.h instead, listed before
// this directory in its own Makefile's -I order so its file is found
// first. This interface is genuinely board-scoped, not ISA-scoped,
// unlike arch_vm.h/arch_proc.h.
#include "devices/storage/interface.h"

void arch_disk_rw(struct buf *b, int write) {
  virtio_disk_rw(b, write);
}

#endif /* ARCH_DISK_H */
