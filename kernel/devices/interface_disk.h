#ifndef INTERFACE_DISK_H
#define INTERFACE_DISK_H

// claude: documentation only - never #included by any build. C has no
// way to declare or enforce an interface, so this file exists purely
// so a reader can find, in one place, the contract every fork's own
// kernel/devices/<arch>/arch_disk.h is expected to satisfy. See
// docs/claude_notes/notes_new_kernel_organization.md for the
// "interface, not permanent fork" method this documents, and any one
// real arch_disk.h (e.g. kernel/devices/arm64/arch_disk.h) for a
// worked example, including the one real case where the choice of
// backend is board-scoped rather than ISA-scoped.
//
// Adopted by 5 of 14 forks so far (arm64, arm64-pi4, loongarch,
// riscv32, riscv64) - the same "MIT-2019 valid/disk-boolean" family
// that already shares kernel/bio.c and kernel/buf.h. The other 9
// forks' own bio.c/bio-legacy.c/bio-x86.c call their disk driver
// (ide.c, virtio_disk.c, memide.c, ...) directly and have no
// interface point here yet.

// Read or write buf `b`'s own 512-byte block to/from this port's real
// disk device - `write` nonzero means write buf->data to disk,
// zero means read disk into buf->data. Every implementation so far is
// a one-line #define pass-through to a real driver function
// (virtio_disk_rw() on the QEMU-virt-backed ports, ramdiskrw() on
// arm64-pi4's real-hardware board, which has no easily-accessible
// virtio device) rather than a real function - cheap enough that the
// macro form was kept instead of promoting it to a proper prototype.
// Called from: kernel/bio.c.
#define arch_disk_rw(b, write) /* ... */

#endif /* INTERFACE_DISK_H */
