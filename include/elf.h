// Format of an ELF executable file.
//
// ELF32 and ELF64 are genuinely two different on-disk formats, not just a
// per-fork drift to reconcile - e_entry/e_phoff/e_shoff are 32-bit in one
// and 64-bit in the other, and Elf64_Phdr also moves p_flags right after
// p_type, where Elf32_Phdr has it between p_memsz and p_align. So this is
// two structs with two names (elf32hdr/elf64hdr, proghdr32/proghdr64),
// not one struct behind a macro switch: each fork's own exec.c/vm.c just
// uses whichever pair matches its own word size.
#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4

// 32-bit file header
struct elf32hdr {
  uint magic;  // must equal ELF_MAGIC
  uchar elf[12];
  ushort type;
  ushort machine;
  uint version;
  uint entry;
  uint phoff;
  uint shoff;
  uint flags;
  ushort ehsize;
  ushort phentsize;
  ushort phnum;
  ushort shentsize;
  ushort shnum;
  ushort shstrndx;
};

// 32-bit program section header
struct proghdr32 {
  uint type;
  uint off;
  uint vaddr;
  uint paddr;
  uint filesz;
  uint memsz;
  // claude: "flags" goes HERE, between memsz and align, matching the real
  // Elf32_Phdr - riscv32's own copy of this file once had it missing
  // entirely, so sizeof(struct proghdr) was 28 bytes instead of the true
  // 32-byte on-disk stride: exec.c's phdr loop advances "off" by
  // sizeof(ph) per iteration, so every phdr after the first was read 4
  // bytes short of where it actually starts, garbling every field
  // (observed as a phantom "vaddr=0x1000 filesz=0" for what should have
  // been two distinct, correctly-sized LOAD segments).
  uint flags;
  uint align;
};

// 64-bit file header
struct elf64hdr {
  uint magic;  // must equal ELF_MAGIC
  uchar elf[12];
  ushort type;
  ushort machine;
  uint version;
  uint64 entry;
  uint64 phoff;
  uint64 shoff;
  uint flags;
  ushort ehsize;
  ushort phentsize;
  ushort phnum;
  ushort shentsize;
  ushort shnum;
  ushort shstrndx;
};

// 64-bit program section header
struct proghdr64 {
  uint32 type;
  uint32 flags;
  uint64 off;
  uint64 vaddr;
  uint64 paddr;
  uint64 filesz;
  uint64 memsz;
  uint64 align;
};
