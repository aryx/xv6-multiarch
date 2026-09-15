#ifndef INTERFACE_DISK_H
#define INTERFACE_DISK_H

// claude: each fork's own kernel/devices/storage/<arch>/arch_disk.h
// #includes this file right before defining arch_disk_rw() as a real
// (non-static) function - safe since arch_disk.h is only ever
// included from the one shared kernel/devices/storage/bio.c, so no
// ODR risk. See docs/claude_notes/notes_new_kernel_organization.md
// for the "interface, not permanent fork" method this documents, and
// any one real arch_disk.h (e.g. kernel/devices/storage/arm64/
// arch_disk.h) for a worked example, including the one real case
// where the choice of backend is board-scoped rather than ISA-scoped.
//
// Adopted by 5 of 14 forks (arm64, arm64-pi4, loongarch, riscv32,
// riscv64) - the same "MIT-2019 valid/disk-boolean" family that
// already shares kernel/bio.c and kernel/buf.h. The other 9 forks'
// own bio.c/bio-legacy.c/bio-x86.c call their disk driver (ide.c,
// virtio_disk.c, memide.c, ...) directly and have no interface point
// here yet.

// Read or write buf `b`'s own 512-byte block to/from this port's real
// disk device - `write` nonzero means write buf->data to disk, zero
// means read disk into buf->data. Every implementation is a one-line
// pass-through to a real driver function (virtio_disk_rw() on the
// QEMU-virt-backed ports, ramdiskrw() on arm64-pi4's real-hardware
// board, which has no easily-accessible virtio device).
// Called from: kernel/devices/storage/bio.c.
void arch_disk_rw(struct buf *b, int write);

#endif /* INTERFACE_DISK_H */
