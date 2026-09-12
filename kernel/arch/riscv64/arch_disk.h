#ifndef ARCH_DISK_H
#define ARCH_DISK_H

// claude: this port's disk is virtio - a plain pass-through backend
// for the shared kernel/bio.c's own disk_rw(b, write) interface. See
// kernel/arch/arm64/arch_disk.h's own comment for the other real
// backend (a ramdisk, no virtio device) this interface exists for.
#define disk_rw(b, write) virtio_disk_rw((b), (write))

#endif /* ARCH_DISK_H */
