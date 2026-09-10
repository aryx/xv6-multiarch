/*****************************************************************
*       console.c
*       adapted from MIT xv6 by Zhiyi Huang, hzy@cs.otago.ac.nz
*       University of Otago
*
********************************************************************/



#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "arm.h"

#define BACKSPACE 0x100

static int panicked = 0;

static struct {
  struct spinlock lock;
  int locking;
} cons;


uint cursor_x=0, cursor_y=0;
uint frameheight=768, framewidth=1024, framecolors=16;
uint fontheight=16, fontwidth=8;
FBI fbinfo __attribute__ ((aligned (16), nocommon));


extern u8 font[];
static uint gpucolour=0xffff;

// claude: true once initframebuf() has produced a framebuffer this
// kernel can actually draw through; gpuputc() returns immediately when
// it's false, so a failed allocation degrades to UART-only console
// output instead of faulting through an unusable pointer.
static uint fb_ready;

// claude: the VIRTUAL address to draw through - not necessarily
// fbinfo.fbp itself. Real VideoCore firmware answers the channel-1
// request with a VC BUS address (the 0x40000000 cache alias), which
// mmu.c already identity-maps via GPUMEMBASE/GPUMEMSIZE, so there
// fb_base == fbinfo.fbp and nothing changes. QEMU's own bcm2835-fb
// model instead answers with a plain ARM PHYSICAL address (0x1c100000
// on -M raspi1ap: just above the 448MB it gives the ARM, inside the
// GPU's own carve-out), which no mapping covers - hence the extra
// mapping step in consoleinit() below.
static uint fb_base;

void setgpucolour(u16 c)
{
    gpucolour = c;
}

uint initframebuf(uint width, uint height, uint depth)
{
    fbinfo.width = width;
    fbinfo.height = height;
    fbinfo.v_width = width;
    fbinfo.v_height = height;
    fbinfo.pitch = 0;
    fbinfo.depth = depth;
    fbinfo.x = 0;
    fbinfo.y = 0;
    fbinfo.fbp = 0;
    fbinfo.fbs = 0;
    writemailbox((uint *)&fbinfo, 1);
    return readmailbox(1);
}

#define INPUT_BUF 128
struct {
  struct spinlock lock;
  char buf[INPUT_BUF];
  uint r;  // Read index
  uint w;  // Write index
  uint e;  // Edit index
} input;

int
consolewrite(struct inode *ip, char *buf, int n)
{
  int i;

//  cprintf("consolewrite is called: ip=%x buf=%x, n=%x", ip, buf, n);
  iunlock(ip);
  acquire(&cons.lock);
  // claude: uartputc() before gpuputc() (here and below) - see
  // uart.c's own pl011putc() comment. gpuputc()/drawpixel() can fault
  // (a real, pre-existing bug: tvinit(), which installs this kernel's
  // own exception vector table, isn't called until well after
  // consoleinit()/the first console output - see main.c's own
  // ordering - so an early framebuffer fault is mishandled by
  // whatever was at the vector table location before tvinit() ran, not
  // this kernel's own trap()/panic()), so put the character on the
  // wire first, in case gpuputc() doesn't return.
  for(i = 0; i < n; i++){
    uartputc(buf[i] & 0xff);
    gpuputc(buf[i] & 0xff);
  }
  release(&cons.lock);
  ilock(ip);

  return n;
}


void drawpixel(uint x, uint y)
{
    u16 *addr;

    if(x >= framewidth || y >= frameheight) return;
    addr = (u16 *) fb_base;
//    addr = (u16 *) ((FBI *)FrameBufferInfo)->fbp;
    addr += y*1024 + x;
    *addr = gpucolour;
    return;
}


void drawcursor(uint x, uint y)
{
u8 row, bit;

    for(row=0; row<15; row++)
        for(bit=0; bit<8; bit++)
            drawpixel(x+bit, y+row);
}

void drawcharacter(u8 c, uint x, uint y)
{
u8 *faddr;
u8 row, bit, bits;
uint tv;

    if(c > 127) return;
    tv = ((uint)c) << 4;
    faddr = font + tv;
    for(row=0; row<15; row++){
        bits = *(faddr+row);
        for(bit=0; bit<8; bit++){
            if((bits>>bit) & 1) drawpixel(x+bit, y+row);
        }
    }

}

//static void
void
gpuputc(uint c)
{
    if(!fb_ready) return;

    if(c=='\n'){
	cursor_x = 0;
	cursor_y += fontheight;
	if(cursor_y >= frameheight) {
		memmove((u8 *)fb_base, (u8 *)fb_base+framewidth*fontheight*2, (frameheight - fontheight)*framewidth*2);
		cursor_y = frameheight - fontheight;
		setgpucolour(0);
		while(cursor_x < framewidth) {
		    drawcursor(cursor_x, cursor_y);
		    cursor_x = cursor_x + fontwidth;
		}
		setgpucolour(0xffff);
		cursor_x = 0;
	}
    } else if(c == BACKSPACE) {
	if (cursor_x > 0) {
		cursor_x -= fontwidth;
		setgpucolour(0);
		drawcursor(cursor_x, cursor_y);
		setgpucolour(0xffff);
	}
    } else {
	setgpucolour(0);
	drawcursor(cursor_x, cursor_y);
	setgpucolour(0xffff);
	if(c!=' ') drawcharacter(c, cursor_x, cursor_y);
	cursor_x = cursor_x + fontwidth;
	if(cursor_x >= framewidth) {
	    cursor_x = 0;
	    cursor_y += fontheight;
	    if(cursor_y >= frameheight) {
		memmove((u8 *)fb_base, (u8 *)fb_base+framewidth*fontheight*2, (frameheight - fontheight)*framewidth*2);
		cursor_y = frameheight - fontheight;
		setgpucolour(0);
		while(cursor_x < framewidth) {
		    drawcursor(cursor_x, cursor_y);
		    cursor_x = cursor_x + fontwidth;
		}
		setgpucolour(0xffff);
		cursor_x = 0;
	    }
	}
    }

}


static void
printint(int xx, int base, int sign)
{
  static u8 digits[] = "0123456789abcdef";
  u8 buf[16];
  int i;
  uint x, y, b;

  if(sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  b = base;
  i = 0;
  do{
    y = div(x, b);
    buf[i++] = digits[x - y * b];
  }while((x = y) != 0);

  if(sign)
    buf[i++] = '-';

  while(--i >= 0){
    uartputc(buf[i]);
    gpuputc(buf[i]);
  }
}


// Print to the console. only understands %d, %x, %p, %s.
void
cprintf(char *fmt, ...)
{
  int i, c;
  int locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if(locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint *)(void*)(&fmt + 1);
  for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
    if(c != '%'){
        uartputc(c);
	gpuputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if(c == 0)
      break;
    switch(c){
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if((s = (char*)*argp++) == 0)
        s = "(null)";
      for(; *s; s++){
        uartputc(*s);
	gpuputc(*s);
      }
      break;
    case '%':
	uartputc('%');
	gpuputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
	uartputc('%');
	gpuputc('%');
	uartputc(c);
	gpuputc(c);
      break;
    }
  }
  if(locking)
    release(&cons.lock);
}

void
panic(char *s)
{
  int i;
  uint pcs[10];

  cprintf("cpu%d: panic: ", 0);
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for(i=0; i<10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU

  for(;;)
    ;
}

#define C(x)  ((x)-'@')  // Control-x

void
consputc(int c)
{
  if(panicked){
    cli();
    for(;;)
      ;
  }

  if(c == BACKSPACE){
    uartputc('\b'); uartputc(' '); uartputc('\b');
    gpuputc('\b'); gpuputc(' '); gpuputc('\b');
  } else if(c == C('D')) {
    uartputc('^'); uartputc('D');
    gpuputc('^'); gpuputc('D');
  } else {
    uartputc(c);
    gpuputc(c);
  }
}


void
consoleintr(int (*getc)(void))
{
  int c;

  acquire(&input.lock);
  while((c = getc()) >= 0){
    switch(c){
    case C('P'):  // Process listing.
      procdump();
      break;
    case C('U'):  // Kill line.
      while(input.e != input.w &&
            input.buf[(input.e-1) % INPUT_BUF] != '\n'){
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    case C('H'): case '\x7f':  // Backspace
      if(input.e != input.w){
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    default:
      if(c != 0 && input.e-input.r < INPUT_BUF){
	if(c == 0xa) break;
        c = (c == 0xd) ? '\n' : c;
        input.buf[input.e++ % INPUT_BUF] = c;
        consputc(c);
        if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
          input.w = input.e;
          wakeup(&input.r);
        }
      }
      break;
    }
  }
  release(&input.lock);
}

int
consoleread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

//cprintf("inside consoleread\n");
  iunlock(ip);
  target = n;
  acquire(&input.lock);
  while(n > 0){
    while(input.r == input.w){
      if(curr_proc->killed){
        release(&input.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &input.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if(c == C('D')){  // EOF
      if(n < target){
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if(c == '\n')
      break;
  }
  release(&input.lock);
  ilock(ip);

  return target - n;
}

void consoleinit(void)
{
uint fbinfoaddr;

  fbinfoaddr = initframebuf(framewidth, frameheight, framecolors);
  if(fbinfoaddr != 0) NotOkLoop();

  // claude: the channel-1 reply's fbp field comes back in one of two
  // forms depending on who answered, so pick the drawing address to
  // match rather than assuming either one (see fb_base's own comment):
  //
  //  - a VC BUS address (real VideoCore firmware: the 0x40000000 cache
  //    alias). mmu.c already identity-maps that whole window, so use it
  //    exactly as before - this branch leaves the real-hardware path
  //    bit-for-bit unchanged.
  //  - a plain ARM PHYSICAL address (QEMU's bcm2835-fb model answers
  //    this way). Nothing maps it: it sits above the RAM window mmu.c
  //    maps at KERNBASE, and outside the GPUMEMBASE identity window. So
  //    map it here, at GPUMEMBASE+phys - the same virtual address real
  //    firmware would have handed back for that physical page, and
  //    deliberately inside the kernel half of the L1 table (VA >=
  //    0x40000000), which switchuvm()'s user-half memmove never
  //    overwrites.
  if(fbinfo.fbp >= GPUMEMBASE && fbinfo.fbp < (uint)GPUMEMBASE+(uint)GPUMEMSIZE){
    fb_base = fbinfo.fbp;
    fb_ready = 1;
  } else if(fbinfo.fbp != 0 && fbinfo.fbs != 0 &&
            fbinfo.fbp < GPUMEMBASE && fbinfo.fbp+fbinfo.fbs <= GPUMEMSIZE){
    pde_t *l1 = (pde_t *)P2V(K_PDX_BASE);
    uint pa, va;

    for(pa = fbinfo.fbp & ~(MBYTE-1); pa < fbinfo.fbp+fbinfo.fbs; pa += MBYTE){
      va = GPUMEMBASE + pa;
      l1[PDX(va)] = pa|DOMAIN0|PDX_AP(K_RW)|SECTION;
    }
    dsb_barrier();
    flush_tlb();
    fb_base = GPUMEMBASE + fbinfo.fbp;
    fb_ready = 1;
  } else {
    fb_ready = 0;
  }

  initlock(&cons.lock, "console");
  memset(&input, 0, sizeof(input));
  initlock(&input.lock, "input");

  memset(devsw, 0, sizeof(struct devsw)*NDEV);
  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;
  panicked = 0; // must initialize in code since the compiler does not

  cursor_x=cursor_y=0;
}

