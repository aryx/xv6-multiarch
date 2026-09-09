/*****************************************************************
*       csud_glue.c
*       claude: glue between this kernel and the vendored CSUD USB
*       driver (csud/, Alex Chadwick's "Chadderz's Simple USB Driver")
*
*       CSUD is built with TYPE_LOWLEVEL, which leaves logging to the
*       host application: csud/source/platform/platform.c implements
*       LogPrintF (its own printf-alike) on top of a LogPrint that it
*       does NOT define. The prebuilt libcsud.a this fork used to ship
*       resolved that by having been rebuilt against xv6's own cprintf;
*       now that the source is vendored, this file supplies the same
*       hook explicitly instead.
*
*       Routed to uartputc() rather than cprintf() on purpose: USB
*       enumeration runs early and CSUD calls this from inside its own
*       transfer paths, so going straight to the UART avoids taking
*       cons.lock (cprintf does) from a context that may already hold
*       it, and avoids the framebuffer path entirely.
********************************************************************/

#include "types.h"
#include "defs.h"

void
LogPrint(const char *message, uint messageLength)
{
  uint i;

  if(message == 0)
    return;
  for(i = 0; i < messageLength; i++){
    if(message[i] == 0)   /* CSUD passes sizeof(literal), so trailing NUL */
      break;
    uartputc(message[i] & 0xff);
  }
}
