// claude: BSIZE is tied to this fork's own disk driver (ide.c's own
// sector_per_block assumption) - not safe to share across forks with
// different drivers. See include/kernel/fs.h's own header comment and
// forks/amd64-jserv/kernel/fs.h's history for why.
#define BSIZE 512
