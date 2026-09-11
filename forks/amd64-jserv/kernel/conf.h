// claude: BSIZE is tied to this fork's own disk driver (ide.c's own
// sector_per_block assumption, see this file's sibling kernel/fs.h
// history) - not safe to share across forks with different drivers.
#define BSIZE 512
