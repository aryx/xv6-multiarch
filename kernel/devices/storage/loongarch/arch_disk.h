#ifndef ARCH_DISK_H
#define ARCH_DISK_H

// claude: this port's disk is a ramdisk, not virtio (no easily-
// accessible real disk controller on this board) - the backend for
// the shared kernel/devices/storage/bio.c's own arch_disk_rw()
// interface. See kernel/devices/storage/arm64/arch_disk.h's own
// comment for the virtio case.
#include "devices/interface_disk.h"

void arch_disk_rw(struct buf *b, int write) {
  ramdiskrw(b, write);
}

#endif /* ARCH_DISK_H */
