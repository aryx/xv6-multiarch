#ifndef ARCH_DISK_H
#define ARCH_DISK_H

// claude: this port's disk is a ramdisk, not virtio (no easily-
// accessible real disk controller on this board) - the backend for
// the shared kernel/bio.c's own disk_rw(b, write) interface. See
// kernel/arch/arm64/arch_disk.h's own comment for the virtio case.
#define disk_rw(b, write) ramdiskrw((b), (write))

#endif /* ARCH_DISK_H */
