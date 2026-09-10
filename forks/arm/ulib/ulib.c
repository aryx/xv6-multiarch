#include "types.h"
#include "stat.h"
#include "fcntl.h"
#include "user.h"

char*
strcpy(char *s, char *t)
{
    char *os;
    
    os = s;
    while((*s++ = *t++) != 0)
        ;
    return os;
}

int
strcmp(const char *p, const char *q)
{
    while(*p && *p == *q)
        p++, q++;
    return (uchar)*p - (uchar)*q;
}

uint
strlen(char *s)
{
    int n;
    
    for(n = 0; s[n]; n++)
        ;
    return n;
}

void*
memset(void *dst, int v, uint n)
{
	uint8	*p;
	uint8	c;
	uint32	val;
	uint32	*p4;

	p   = dst;
	c   = v & 0xff;
	val = (c << 24) | (c << 16) | (c << 8) | c;

	// set bytes before whole uint32
	for (; (n > 0) && ((uint)p % 4); n--, p++){
		*p = c;
	}

	// set memory 4 bytes a time
	p4 = (uint*)p;

	for (; n >= 4; n -= 4, p4++) {
		*p4 = val;
	}

	// set leftover one byte a time
	p = (uint8*)p4;

	for (; n > 0; n--, p++) {
		*p = c;
	}

	return dst;
}

char*
strchr(const char *s, char c)
{
    for(; *s; s++)
        if(*s == c)
            return (char*)s;
    return 0;
}

char*
gets(char *buf, int max)
{
    int i, cc;
    char c;
    
    for(i=0; i+1 < max; ){
        cc = read(0, &c, 1);
        if(cc < 1)
            break;
        buf[i++] = c;
        if(c == '\n' || c == '\r')
            break;
    }
    buf[i] = '\0';
    return buf;
}

int
stat(char *n, struct stat *st)
{
    int fd;
    int r;
    
    fd = open(n, O_RDONLY);
    if(fd < 0)
        return -1;
    r = fstat(fd, st);
    close(fd);
    return r;
}

int
atoi(const char *s)
{
    int n;
    
    n = 0;
    while('0' <= *s && *s <= '9')
        n = n*10 + *s++ - '0';
    return n;
}

void*
memmove(void *vdst, void *vsrc, int n)
{
    char *dst, *src;

    dst = vdst;
    src = vsrc;
    while(n-- > 0)
        *dst++ = *src++;
    return vdst;
}

// claude: minimal libgcc-runtime-support stub, not a real signal
// implementation - this freestanding, libc-less user environment has no
// definition for "raise" at all, but any 32-bit integer division
// (__aeabi_idiv/__aeabi_uidiv, used all over printf.c's own number
// formatting) statically pulls in libgcc's shared "_dvmd_lnx.o" object,
// which ALSO defines the (unused-in-practice, but still present and
// referenced) 64-bit division-by-zero trap handler __aeabi_ldiv0 - and
// that calls raise() unconditionally. A real division by zero should
// never happen in this codebase; if it somehow did, exiting is a
// reasonable fallback (matches this port's own "NotOkLoop"-style fatal
// error handling elsewhere).
int
raise(int sig)
{
    exit(0);
    return 0; // unreached
}
