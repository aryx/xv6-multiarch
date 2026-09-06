// Format of an ELF executable file

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// File header
struct elfhdr {
  uint magic;  // must equal ELF_MAGIC
  uchar elf[12];
  ushort type;
  ushort machine;
  uint32 version;
  uint32 entry;
  uint32 phoff;
  uint32 shoff;
  uint32 flags;
  ushort ehsize;
  ushort phentsize;
  ushort phnum;
  ushort shentsize;
  ushort shnum;
  ushort shstrndx;
};

// Program section header
struct proghdr {
  uint32 type;
  uint32 off;
  uint32 vaddr;
  uint32 paddr;
  uint32 filesz;
  uint32 memsz;
  // claude: the real Elf32_Phdr has "flags" here, between memsz and align
  // (see forks/x86/elf.h's own struct proghdr, also ELF32, which has it) -
  // this field was missing entirely, so sizeof(struct proghdr) was 28
  // bytes instead of the true 32-byte on-disk stride. exec.c's phdr loop
  // advances "off" by sizeof(ph) per iteration, so every phdr after the
  // first was read 4 bytes short of where it actually starts - garbling
  // every field (observed as a phantom "vaddr=0x1000 filesz=0" reading for
  // what should have been two distinct, correctly-sized LOAD segments).
  uint32 flags;
  uint32 align;
};

// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4
