#include "board.h"

#ifdef CYGDBG_HAL_DEBUG_GDB_INCLUDE_STUBS

/* Eventually, this should default to ON */
#if USE_GDBSTUB_PROTOTYPES
#include "stub-tservice.h"
#include "generic-stub.h"
#else
// Function declarations (prevents compiler warnings)
int stubhex (unsigned char ch);
static void getpacket (char *buffer);
static void unlock_thread_scheduler (void);
static uint32 crc32 (unsigned char *ptr, int len, uint32 crc);
#endif

#include "thread-pkts.h"
  /* Defines function macros if thread support is not selected in board.h */


/****************************************************************************

                THIS SOFTWARE IS NOT COPYRIGHTED

   HP offers the following for use in the public domain.  HP makes no
   warranty with regard to the software or it's performance and the
   user accepts the software "AS IS" with all faults.

   HP DISCLAIMS ANY WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD
   TO THIS SOFTWARE INCLUDING BUT NOT LIMITED TO THE WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

****************************************************************************/

/****************************************************************************
 *  Header: remcom.c,v 1.34 91/03/09 12:29:49 glenne Exp $
 *
 *  Module name: remcom.c $
 *  Revision: 1.34 $
 *  Date: 91/03/09 12:29:49 $
 *  Contributor:     Lake Stevens Instrument Division$
 *
 *  Description:     low level support for gdb debugger. $
 *
 *  Considerations:  only works on target hardware $
 *
 *  Written by:      Glenn Engel $
 *  ModuleState:     Experimental $
 *
 *  NOTES:           See Below $
 *
 *  Modified for SPARC by Stu Grossman, Cygnus Solutions.
 *  Modified for generic CygMON stub support by Bob Manson, Cygnus Solutions.
 *
 *  To enable debugger support, two things need to happen.  One, a
 *  call to set_debug_traps () is necessary in order to allow any breakpoints
 *  or error conditions to be properly intercepted and reported to gdb.
 *  Two, a breakpoint needs to be generated to begin communication.  This
 *  is most easily accomplished by a call to breakpoint ().  Breakpoint ()
 *  simulates a breakpoint by executing a trap #1.
 *
 *************
 *
 *    The following gdb commands are supported:
 *
 * command          function                               Return value
 *
 *    g             return the value of the CPU registers  hex data or ENN
 *    G             set the value of the CPU registers     OK or ENN
 *
 *    mAA..AA,LLLL  Read LLLL bytes at address AA..AA      hex data or ENN
 *    MAA..AA,LLLL: Write LLLL bytes at address AA.AA      OK or ENN
 *
 *    c             Resume at current address              SNN   ( signal NN)
 *    cAA..AA       Continue at address AA..AA             SNN
 *
 *    s             Step one instruction                   SNN
 *    sAA..AA       Step one instruction from AA..AA       SNN
 *
 *    k             kill
 *
 *    ?             What was the last sigval ?             SNN   (signal NN)
 *
 *    bBB..BB       Set baud rate to BB..BB                OK or BNN, then sets
 *                                                         baud rate
 *
 * All commands and responses are sent with a packet which includes a
 * checksum.  A packet consists of
 *
 * $<packet info>#<checksum>.
 *
 * where
 * <packet info> :: <characters representing the command or response>
 * <checksum>    :: < two hex digits computed as modulo 256 sum of <packetinfo>>
 *
 * When a packet is received, it is first acknowledged with either '+' or '-'.
 * '+' indicates a successful transfer.  '-' indicates a failed transfer.
 *
 * Example:
 *
 * Host:                  Reply:
 * $m0,10#2a               +$00010203040506070809101112131415#42
 *
 ****************************************************************************/

#ifndef __ECOS__
#include <string.h>
#include <signal.h>
#endif // __ECOS__

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 2048

static int initialized = 0;     /* !0 means we've been initialized */

static int process_exception (int sigval);
static void do_nothing (void); /* and do it gracefully */
static int syscall_do_nothing (int);

void __free_program_args (void);

volatile __PFI __process_exception_vec = process_exception;
volatile __PFV __process_exit_vec = do_nothing;
volatile __PFI __process_syscall_vec = syscall_do_nothing;
volatile __PFI __process_signal_vec = NULL;
volatile __PFV __init_vec = NULL;
volatile __PFV __cleanup_vec = NULL;

static char *__add_program_arg (int argnum, uint32 arglen);

static const char hexchars[] = "0123456789abcdef";

static void process_query (char *pkt);
static void process_set   (char *pkt);

char
__tohex (int c)
{
  return hexchars [c & 15];
}

#define __tohex(c) hexchars[(c) & 15]

#ifndef NUMREGS_GDB
#define NUMREGS_GDB NUMREGS
#endif

/* One pushback character. */
int ungot_char = -1;

static int
readDebugChar (void)
{
  if (ungot_char > 0)
    {
      int result = ungot_char;
      ungot_char = -1;
      return result;
    }
  else
    return getDebugChar ();
}

/* Convert ch from a hex digit to an int. */

int
stubhex (ch)
     unsigned char ch;
{
  if (ch >= 'a' && ch <= 'f')
    return ch-'a'+10;
  if (ch >= '0' && ch <= '9')
    return ch-'0';
  if (ch >= 'A' && ch <= 'F')
    return ch-'A'+10;
  return -1;
}

static void
getpacket (buffer)
     char *buffer;
{
  struct gdb_packet packet;

  packet.state = 0;
  packet.contents = buffer;
  while (__add_char_to_packet (readDebugChar () & 0xff, &packet) != 1)
    {
      /* Empty */
    }
}

int
__add_char_to_packet (ch, packet)
     unsigned int ch;
     struct gdb_packet *packet;
{
  if (packet->state == 0)
    {
      if (ch == '$')
        {
          packet->state = 1;
          packet->length = 0;
          packet->checksum = 0;
          packet->xmitcsum = -1;
        }
      return 0;
    }
  
  if (packet->state == 1)
    {
      if (ch == '#')
        {
          packet->contents[packet->length] = 0;
          packet->state = 2;
        }
      else
        {
          if (packet->length == BUFMAX) {
            packet->state = 0;
            return -1;
          }
          packet->checksum += ch;
          packet->contents[packet->length++] = ch;
        }
      return 0;
    }

  if (packet->state == 2)
    {
      packet->xmitcsum = stubhex (ch) << 4;
      packet->state = 3;
      return 0;
    }

  if (packet->state == 3)
    {
      packet->xmitcsum |= stubhex (ch);
      if ((packet->checksum & 255) != packet->xmitcsum)
        {
          putDebugChar ('-');   /* failed checksum */
          packet->state = 0;
          return -1;
        }
      else
        {
          putDebugChar ('+'); /* successful transfer */
          /* if a sequence char is present, reply the sequence ID */
          if (packet->contents[2] == ':')
            {
              uint32 count = packet->length;
              uint32 i;
              putDebugChar (packet->contents[0]);
              putDebugChar (packet->contents[1]);
              /* remove sequence chars from buffer */
              for (i=3; i <= count; i++)
                packet->contents[i-3] = packet->contents[i];
            }
          return 1;
        }
    }
  /* We should never get here. */
  packet->state = 0;
  return -1;
}

/* send the packet in buffer.  */

void
__putpacket (buffer)
     char *buffer;
{
  unsigned char checksum;
  uint32 count;
  unsigned char ch;

  /*  $<packet info>#<checksum>. */
  do
    {
      putDebugChar ('$');
      checksum = 0;
      count = 0;

      while ((ch = buffer[count]))
        {
          putDebugChar (ch);
          checksum += ch;
          count += 1;
        }

      putDebugChar ('#');
      putDebugChar (hexchars[(checksum >> 4) & 0xf]);
      putDebugChar (hexchars[checksum & 0xf]);

    }
  while ((readDebugChar () & 0x7f) != '+');
}

static char remcomInBuffer[BUFMAX];
static char remcomOutBuffer[BUFMAX];

/* Indicate to caller of mem2hex or hex2mem that there has been an
   error.  */
volatile int __mem_fault = 0;


#ifndef TARGET_HAS_OWN_MEM_FUNCS
/*
 * _target_readmem_hook / _target_writemem_hook:
 * Allow target to get involved in reading/writing memory.
 *
 * If these hooks are defined by the target, they will be
 * called for each user program memory access.  Otherwise, the stub
 * will simply dereference a pointer to access user program memory.
 */

unsigned char (*_target_readmem_hook)  (unsigned char* addr);
void          (*_target_writemem_hook) (unsigned char* addr, 
                                        unsigned char value);

static unsigned char
get_target_byte (volatile unsigned char *address)
{
  if (_target_readmem_hook)     /* target needs to control memory access */
    return _target_readmem_hook ((unsigned char *) address);
  else
    return *address;
}

static void
put_target_byte (volatile unsigned char *address, unsigned char value)
{
  if (_target_writemem_hook)    /* target needs to control memory access */
    _target_writemem_hook ((unsigned char *) address, value);
  else
    *address = value;
}

/* These are the "arguments" to __do_read_mem and __do_write_mem, 
   which are passed as globals to avoid squeezing them thru
   __set_mem_fault_trap.  */

static volatile target_register_t memCount;
static volatile unsigned char    *memSrc,  *memDst;

/*
 * __do_read_mem:
 * Copy from target memory to trusted memory.
 */

static void
__do_read_mem (void)
{
  __mem_fault = 0;
  while (memCount)
    {
      unsigned char ch = get_target_byte (memSrc++);

      if (__mem_fault)
        return;
      *memDst++ = ch;
      memCount--;
    }
}

/*
 * __do_write_mem:
 * Copy from trusted memory to target memory.
 */

static void
__do_write_mem (void)
{
  __mem_fault = 0;
  while (memCount)
    {
      unsigned char ch = *memSrc++;

      put_target_byte (memDst++, ch);
      if (__mem_fault)
        return;
      memCount--;
    }
}

/*
 * __read_mem_safe:
 * Get contents of target memory, abort on error.
 */

int
__read_mem_safe (void *dst, target_register_t src, int count)
{
  memCount = count;
  memSrc   = (unsigned char *) src;
  memDst   = (unsigned char *) dst;
  __set_mem_fault_trap (__do_read_mem);
  return count - memCount;      /* return number of bytes successfully read */
}

/*
 * __write_mem_safe:
 * Set contents of target memory, abort on error.
 */

int
__write_mem_safe (unsigned char *src, target_register_t dst, int count)
{
  memCount = count;
  memSrc   = (unsigned char *) src;
  memDst   = (unsigned char *) dst;
  __set_mem_fault_trap (__do_write_mem);
  return count - memCount;      /* return number of bytes successfully read */
}

#endif /* TARGET_HAS_OWN_MEM_FUNCS */

/* These are the "arguments" to __mem2hex_helper and __hex2mem_helper, 
   which are passed as globals to avoid squeezing them thru
   __set_mem_fault_trap.  */

static int   hexMemCount;
static char *hexMemSrc, *hexMemDst;
static int   may_fault_mode;

/* Hamburger helper? */
static void
__mem2hex_helper (void)
{
    union {
        unsigned long  long_val;
        unsigned char  bytes[sizeof(long)];
    } val;
    int len, i;
    unsigned char ch;
    __mem_fault = 0;
    while (hexMemCount > 0) {
        if (may_fault_mode) {
            if ((hexMemCount >= sizeof(long)) &&
                (((target_register_t)hexMemSrc & (sizeof(long)-1)) == 0)) {
                // Should be safe to access via a long
                len = sizeof(long);
            } else if ((hexMemCount >= sizeof(short)) &&
                       (((target_register_t)hexMemSrc & (sizeof(short)-1)) == 0)) {
                // Should be safe to access via a short
                len = sizeof(short);
            } else {
                len = 1;
            }
            __read_mem_safe(&val.bytes[0], hexMemSrc, len);
        } else {
            len = 1;
            val.bytes[0] = *hexMemSrc;
        }
        if (__mem_fault)
            return;

        for (i = 0;  i < len;  i++) {
            ch = val.bytes[i];
            *(hexMemDst++) = hexchars[(ch >> 4) & 0xf];
            if (__mem_fault)
                return;
            *(hexMemDst++) = hexchars[ch & 0xf];
            if (__mem_fault)
                return;
        }
        hexMemCount -= len;
        hexMemSrc += len;
    }
}

/* Convert the memory pointed to by MEM into HEX, placing result in BUF.
 * Return a pointer to the last char put in buf (NUL). In case of a memory
 * fault, return 0.
 * If MAY_FAULT is non-zero, then we will handle memory faults by returning
 * a 0 (and assume that MEM is a pointer into the user program), else we 
 * treat a fault like any other fault in the stub (and assume that MEM is
 * a pointer into the stub's memory).
 */

char *
__mem2hex (mem, buf, count, may_fault)
     char *mem;
     char *buf;
     int count;
     int may_fault;
{
  hexMemDst      = (unsigned char *) buf;
  hexMemSrc      = (unsigned char *) mem;
  hexMemCount    = count;
  may_fault_mode = may_fault;
  
  if (may_fault)
    {
      if (__set_mem_fault_trap (__mem2hex_helper))
        return 0;
    }
  else
    __mem2hex_helper ();

  *hexMemDst = 0;

  return (char *) hexMemDst;
}

static void
__hex2mem_helper (void)
{
    union {
        unsigned long  long_val;
        unsigned char  bytes[sizeof(long)];
    } val;
    int len, i;
    unsigned char ch = '\0';

    __mem_fault = 0;
    while (hexMemCount > 0 && *hexMemSrc) {
        if (may_fault_mode) {
            if ((hexMemCount >= sizeof(long)) &&
                (((target_register_t)hexMemDst & (sizeof(long)-1)) == 0)) {
                len = sizeof(long);
            } else if ((hexMemCount >= sizeof(short)) &&
                       (((target_register_t)hexMemDst & (sizeof(short)-1)) == 0)) {
                len = sizeof(short);
            } else {
                len = 1;
            }
        } else {
            len = 1;
        }

        for (i = 0;  i < len;  i++) {
            // Check for short data?
            ch = stubhex (*(hexMemSrc++)) << 4;
            if (__mem_fault)
                return;
            ch |= stubhex (*(hexMemSrc++));
            if (__mem_fault)
                return;
            val.bytes[i] = ch;
        }

        if (may_fault_mode)
            __write_mem_safe (&val.bytes[0], hexMemDst, len);
        else
            *hexMemDst = ch;

        if (__mem_fault)
            return;
        hexMemCount -= len;
        hexMemDst += len;
    }
}

/* Convert COUNT bytes of the hex array pointed to by BUF into binary
   to be placed in MEM.  Return a pointer to the character AFTER the
   last byte written.

   If MAY_FAULT is set, we will return a non-zero value if a memory
   fault occurs (and we assume that MEM is a pointer into the user
   program). Otherwise, we will take a trap just like any other memory
   fault (and assume that MEM points into the stub's memory). */

char *
__hex2mem (buf, mem, count, may_fault)
     char *buf;
     char *mem;
     int count;
     int may_fault;
{
  hexMemSrc      = (unsigned char *) buf;
  hexMemDst      = (unsigned char *) mem;
  hexMemCount    = count;
  may_fault_mode = may_fault;

  if (may_fault)
    {
      if (__set_mem_fault_trap (__hex2mem_helper))
        return 0;
    }
  else
    __hex2mem_helper ();

  return (char *) hexMemDst;
}

void
set_debug_traps (void)
{
  __install_traps ();
  initialized = 1;    /* FIXME: Change this to dbg_stub_initialized */
}

/*
 * While we find nice hex chars, build an int.
 * Return number of chars processed.
 */

unsigned int
__hexToInt (char **ptr, target_register_t *intValue)
{
  int numChars = 0;
  int hexValue;

  *intValue = 0;

  while (**ptr)
    {
      hexValue = stubhex (**ptr);
      if (hexValue < 0)
        break;

      *intValue = (*intValue << 4) | hexValue;
      numChars ++;

      (*ptr)++;
    }

  return (numChars);
}

/* 
 * Complement of __hexToInt: take an int of size "numBits", 
 * convert it to a hex string.  Return length of (unterminated) output.
 */

unsigned int
__intToHex (char *ptr, target_register_t intValue, int numBits)
{
  int numChars = 0;

  if (intValue == 0)
    {
      *(ptr++) = '0';
      *(ptr++) = '0';
      return 2;
    }

  numBits = (numBits + 7) / 8;
  while (numBits)
    {
      int v = (intValue >> ((numBits - 1) * 8));
      if (v || (numBits == 1))
        {
          v = v & 255;
          *(ptr++) = __tohex ((v / 16) & 15);
          *(ptr++) = __tohex (v & 15);
          numChars += 2;
        }
      numBits--;
    }

  return (numChars);
}

#if DEBUG_THREADS 
/*
 * Kernel Thread Control
 *
 * If the current thread is set to other than zero (or minus one),
 * then ask the kernel to lock it's scheduler so that only that thread
 * can run.
 */

static unsigned char did_lock_scheduler = 0;
static unsigned char did_disable_interrupts = 0;

/* Pointer to "kernel call" for scheduler control */
static int (*schedlock_fn) (int, int, long) = stub_lock_scheduler;

/* Pointer to target stub call for disabling interrupts.
   Target stub will initialize this if it can.  */
int (*__disable_interrupts_hook) (int); /* don't initialize here! */
#endif

static void
lock_thread_scheduler (int kind)        /* "step" or "continue" */
{
#if DEBUG_THREADS 
  int ret = 0;

  /* GDB will signal its desire to run a single thread
     by setting _gdb_cont_thread to non-zero / non-negative.  */
  if (_gdb_cont_thread <= 0)
    return;

  if (schedlock_fn)                     /* kernel call */
    ret = (*schedlock_fn) (1, kind, _gdb_cont_thread);

  if (ret == 1)
    {
      did_lock_scheduler = 1;
      return;
    }

  if (schedlock_fn == 0 ||              /* no kernel scheduler call */
      ret == -1)                        /* kernel asks stub to handle it */
    if (__disable_interrupts_hook)      /* target stub has capability */
      if ((*__disable_interrupts_hook) (1))
        {
          did_disable_interrupts = 1;
          return;
        }
#endif /* DEBUG_THREADS */
}

static void
unlock_thread_scheduler ()
{
#if DEBUG_THREADS
  if (did_lock_scheduler)
    if (schedlock_fn)                   /* kernel call */
      {
        (*schedlock_fn) (0, 0, _gdb_cont_thread);
        /* I could check the return value, but 
           what would I do if it failed???  */
        did_lock_scheduler = 0;
      }
  if (did_disable_interrupts)
    if (__disable_interrupts_hook)      /* target stub call */
      {
        (*__disable_interrupts_hook) (0);
        /* Again, I could check the return value, but 
           what would I do if it failed???  */
        did_disable_interrupts = 0;
      }
#endif /* DEBUG_THREADS */
}

void
__handle_exception (void)
{
  int sigval;

#ifdef TARGET_HAS_NEXT_STEP
  if (! __next_step_done ())
    {
      __clear_breakpoints ();
      __install_breakpoints ();
      __single_step ();
      return;
    }
#endif

#ifdef __ECOS__
  // We need to unpack the registers before they are accessed.
  if (__cleanup_vec != NULL)
    __cleanup_vec ();

#ifdef CYGDBG_HAL_DEBUG_GDB_BREAK_SUPPORT
  // Special case for GDB BREAKs. This flag is set by cyg_stub_cleanup.
  if (cyg_hal_gdb_break) {
      cyg_hal_gdb_break = 0;
      sigval = SIGINT;
  } else
#endif
      sigval = __computeSignal (__get_trap_number ());
#else  // __ECOS__
  /* reply to host that an exception has occurred */
  sigval = __computeSignal (__get_trap_number ());
#endif // __ECOS__

  if (__is_breakpoint_function ())
    __skipinst ();

#ifndef __ECOS__
  if (__cleanup_vec != NULL)
    __cleanup_vec ();
#endif // !__ECOS__

  __clear_breakpoints (); 

  /* Undo effect of previous single step.  */
  unlock_thread_scheduler ();
  __clear_single_step ();

#ifdef SIGSYSCALL
  if (sigval == SIGSYSCALL)
    {
      int val;
      /* Do the skipinst FIRST. */
#ifndef SYSCALL_PC_AFTER_INST
      __skipinst ();
#endif
      val =  __process_syscall_vec (__get_syscall_num ());
      if (val < 0)
        sigval = -val;
      else
        sigval = 0;
    }

#endif

  /* Indirect function call to stub, cygmon monitor or other */
  if (sigval != 0)
    {
      while (__process_exception_vec (sigval))
        {
          /* Empty! */
        }
    }

  __install_breakpoints ();

  if (__init_vec != NULL)
    __init_vec ();
}

/*
 * _get_trace_register_hook:
 * This function pointer will be non-zero if the trace component
 * wants to intercept requests for register values.
 * 
 * FIXME: evidently I need a new hook for large registers...
 */

int   (*_get_trace_register_hook) (regnames_t, target_register_t *);

int
__process_packet (char *packet)
{
  int  is_binary = 0;
  remcomOutBuffer[0] = 0;

  switch (packet[0])
    {
    case '?':
      {
        int sigval = __computeSignal (__get_trap_number ());
        remcomOutBuffer[0] = 'S';
        remcomOutBuffer[1] = hexchars[(sigval >> 4) & 0xf];
        remcomOutBuffer[2] = hexchars[sigval & 0xf];
        remcomOutBuffer[3] = 0;
        break;
      }

    case 'd':
      /* toggle debug flag */
#ifdef __ECOS__
#if !defined(CYG_HAL_STARTUP_RAM)    // Only for ROM based stubs
      strcpy(remcomOutBuffer, "eCos GDB stubs - built " __DATE__ " / " __TIME__);
#endif
#endif // __ECOS__
      break;

    case 'q':
      /* general query packet */
      process_query (&packet[1]);
      break;

    case 'Q':
      /* general set packet */
      process_set (&packet[1]);
      break;

    case 'g':           /* return the value of the CPU registers */
      {
        char *ptr = remcomOutBuffer;
        int regnum;

        for (regnum = 0; regnum < NUMREGS_GDB; regnum++)
          {
            /* We need to compensate for the value offset within the
               register. */
            char dummyDat[32];
            target_register_t addr;
            char *vptr;
            int  reg_valid = 1;

#ifdef TARGET_HAS_LARGE_REGISTERS
            if (sizeof (target_register_t) < REGSIZE (regnum))
              {
                get_register_as_bytes (regnum, dummyDat);
                vptr = dummyDat;
              }
            else
#endif
              {
                if (_get_trace_register_hook)
                  reg_valid = _get_trace_register_hook (regnum, &addr);
                else
                  addr = get_register (regnum);

                vptr = ((char *) &addr);
                if (sizeof (addr) > REGSIZE(regnum))
                  {
                    /* May need to cope with endian-ness */

#if !defined(__LITTLE_ENDIAN__) && !defined(_LITTLE_ENDIAN)
                    vptr += sizeof (addr) - REGSIZE (regnum);
#endif
                  }
                else if (sizeof (addr) < REGSIZE (regnum))
                  {
                    int off = REGSIZE (regnum) - sizeof (addr);
                    int x;

                    for (x = 0; x < off; x++)
                      dummyDat[x] = 0;
                    memcpy (dummyDat + off, &addr, sizeof (addr));
                    vptr = dummyDat;
                  }
              }
            if (reg_valid)      /* we have a valid reg value */
              {
                ptr = __mem2hex (vptr, ptr, REGSIZE (regnum), 0);
              }
            else
              {
                /* Trace component returned a failure code.
                   This means that the register value is not available.
                   We'll fill it with 'x's, and GDB will understand.  */
                memset (ptr, 'x', 2 * REGSIZE (regnum));
                ptr += 2 * REGSIZE (regnum);
              }
          }
        break;
      }

    case 'A': /* set program arguments */
      {
        if (packet[1] == '\0')
          {
            __free_program_args ();
            strcpy (remcomOutBuffer, "OK");
          }
        else 
          {
            target_register_t arglen, argnum;
            char *ptr = &packet[1];

            while (1)
              {
                if (__hexToInt (&ptr, &arglen)
                    && (*ptr++ == ',')
                    && __hexToInt (&ptr, &argnum)
                    && (*ptr++ == ','))
                  {
                    if (arglen > 0)
                      {
                        char *s = __add_program_arg (argnum, arglen);
                        if (s != NULL)
                          {
                            __hex2mem (ptr, s, arglen, 0);
                          }
                        ptr += arglen * 2;
                      }

                    if (*ptr == ',')
                      ptr++;
                    else
                      break;
                  }
                else
                  break;
              }
            if (*ptr == '\0')
              strcpy (remcomOutBuffer, "OK");
            else
              strcpy (remcomOutBuffer, "E01");
          }
      }
      break;

    case 'P':
    case 'G':      /* set the value of the CPU registers - return OK */
      {
        int x;
        int sr = 0, er = NUMREGS_GDB;
        char *ptr = &packet[1];

        if (packet[0] == 'P')
          {
            target_register_t regno;

            if (__hexToInt (&ptr, &regno)
                && (*ptr++ == '='))
              {
                sr = regno;
                er = regno + 1;
              }
            else
              {
                strcpy (remcomOutBuffer, "P01");
                break;
              }
          }

        for (x = sr; x < er; x++)
          {
            target_register_t value = 0;
            char *vptr;

#ifdef TARGET_HAS_LARGE_REGISTERS
            if (sizeof (target_register_t) < REGSIZE (x))
              {
                char dummyDat [32];

                __hex2mem (ptr, dummyDat, REGSIZE (x), 0);
                put_register_as_bytes (x, dummyDat);
              }
            else
#endif
              {
                vptr = ((char *) &value);
#if !defined(__LITTLE_ENDIAN__) && !defined(_LITTLE_ENDIAN)
                vptr += sizeof (value) - REGSIZE (x);
#endif
                __hex2mem (ptr, vptr, REGSIZE (x), 0);
                put_register (x, value);
              }
            ptr += REGSIZE (x) * 2;
          }

        strcpy (remcomOutBuffer, "OK");
        break;
      }

    case 'm':     /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
      /* Try to read %x,%x.  */
      {
        target_register_t addr, length;
        char *ptr = &packet[1];

        if (__hexToInt (&ptr, &addr)
            && *ptr++ == ','
            && __hexToInt (&ptr, &length))
          {
            if (__mem2hex ((char *) addr, remcomOutBuffer, length, 1))
              break;

            strcpy (remcomOutBuffer, "E03");
          }
        else
          strcpy (remcomOutBuffer, "E01");
        break;
      }

    case 'X':
      /* XAA..AA,LLLL: Write LLLL escaped binary bytes at address AA.AA */
      is_binary = 1;
      /* fall through */
    case 'M': /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA return OK */
      /* Try to read '%x,%x:'.  */
      {
        target_register_t addr, length;
        char *ptr = &packet[1], buf[128];
        int  i;

        if (__hexToInt (&ptr, &addr)
            && *ptr++ == ','
            && __hexToInt (&ptr, &length)
            && *ptr++ == ':')
          {
            if (is_binary)
              {
                while (length > 0)
                  {
                    for (i = 0; i < sizeof(buf) && i < length; i++)
                      if ((buf[i] = *ptr++) == 0x7d)
                        buf[i] = 0x20 | (*ptr++ & 0xff);

                    if (__write_mem_safe (buf, (void *)addr, i) != i)
                      break;

                    length -= i;
                    addr += i;
                  }
                if (length <= 0)
                  strcpy (remcomOutBuffer, "OK");
                else
                  strcpy (remcomOutBuffer, "E03");
              }
              else
              {
                if (__hex2mem (ptr, (char *) addr, length, 1) != NULL)
                  strcpy (remcomOutBuffer, "OK");
                else
                  strcpy (remcomOutBuffer, "E03");
              }
          }
        else
          strcpy (remcomOutBuffer, "E02");
        break;
      }

    case 'S':
    case 's':    /* sAA..AA    Step from address AA..AA (optional) */
    case 'C':
    case 'c':    /* cAA..AA    Continue at address AA..AA (optional) */
      /* try to read optional parameter, pc unchanged if no parm */

      {
        char *ptr = &packet[1];
        target_register_t addr;
        target_register_t sigval = 0;

        if (packet[0] == 'C' || packet[0] == 'S')
          {
            __hexToInt (&ptr, &sigval);
            if (*ptr == ';')
              ptr++;
          }

        if (__hexToInt (&ptr, &addr))
          set_pc (addr);

      /* Need to flush the instruction cache here, as we may have
         deposited a breakpoint, and the icache probably has no way of
         knowing that a data ref to some location may have changed
         something that is in the instruction cache.  */

#ifdef __ECOS__
        __data_cache (CACHE_FLUSH) ;
#endif
        __instruction_cache (CACHE_FLUSH) ;

        /* If we have a function to handle signals, call it. */
        if (sigval != 0 && __process_signal_vec != NULL)
          {
            /* If 0 is returned, we either ignored the signal or invoked a user
               handler. Otherwise, the user program should die. */
            if (! __process_signal_vec (sigval))
              sigval = 0;
          }

        if (sigval != 0)
          {
            sigval = SIGKILL; /* Always nuke the program */
            __kill_program (sigval);
            return 0;
          }

        /* Set machine state to force a single step.  */
        if (packet[0] == 's' || packet[0] == 'S')
          {
            lock_thread_scheduler (0);  /* 0 == single-step */
#ifdef __ECOS__
            // PR 19845 workaround:
            // Make sure the single-step magic affects the correct registers.
            _registers = &registers[0];
#endif
            __single_step ();
          }
        else
          {
            lock_thread_scheduler (1);  /* 1 == continue */
          }

#ifdef __ECOS__
      /* Need to flush the data and instruction cache here, as we may have
         deposited a breakpoint in __single_step. */

        __data_cache (CACHE_FLUSH) ;
        __instruction_cache (CACHE_FLUSH) ;
#endif

        return -1;
      }

      /* kill the program */
    case 'k' :
      __process_exit_vec ();
      return 1;

    case 'r':           /* Reset */
      __reset ();
      break;

    case 'H':
      STUB_PKT_CHANGETHREAD (packet+1, remcomOutBuffer, 300) ;
      break ;
    case 'T' :
      STUB_PKT_THREAD_ALIVE (packet+1, remcomOutBuffer, 300) ;
      break ;
    case 'B':
      /* breakpoint */
      {
        target_register_t addr;
        char mode;
        char *ptr = &packet[1];
        if (__hexToInt (&ptr, &addr) && *(ptr++) == ',')
          {
            mode = *(ptr++);
            if (mode == 'C')
              __remove_breakpoint (addr);
            else
              __set_breakpoint (addr);
            strcpy (remcomOutBuffer, "OK");
          }
        else
          {
            strcpy (remcomOutBuffer, "E01");
          }
        break;
      }

      case 'b':   /* bBB...  Set baud rate to BB... */
      {
        target_register_t baudrate;

        char *ptr = &packet[1];
        if (!__hexToInt (&ptr, &baudrate))
          {
            strcpy (remcomOutBuffer, "B01");
            break;
          }

        __putpacket ("OK");     /* Ack before changing speed */
        __set_baud_rate (baudrate);
        break;
      }
    default:
      __process_target_packet (packet, remcomOutBuffer, 300);
      break;
    }

  /* reply to the request */
  __putpacket (remcomOutBuffer);
  return 0;
}

static void
send_t_packet (int sigval)
{
  __build_t_packet (sigval, remcomOutBuffer);
  __putpacket (remcomOutBuffer);
}

/*
 * This function does all command procesing for interfacing to gdb.
 */

static int
process_exception (int sigval)
{
  int status;

  /* Nasty. */
  if (ungot_char < 0)
    send_t_packet (sigval);

  do {
    getpacket (remcomInBuffer);
    status = __process_packet (remcomInBuffer);
  } while (status == 0);

  if (status < 0)
    return 0;
  else
    return 1;
}

void
__send_exit_status (int status)
{
  remcomOutBuffer[0] = 'W';
  remcomOutBuffer[1] = hexchars[(status >> 4) & 0xf];
  remcomOutBuffer[2] = hexchars[status & 0xf];
  remcomOutBuffer[3] = 0;
  __putpacket (remcomOutBuffer);
}

/* Read up to MAXLEN bytes from the remote GDB client, and store in DEST
   (which is a pointer in the user program). BLOCK indicates what mode
   is being used; if it is set, we will wait for MAXLEN bytes to be
   entered. Otherwise, the function will return immediately with whatever
   bytes are waiting to be read.

   The value returned is the number of bytes read. A -1 indicates that an
   error of some sort occurred. */

int
__get_gdb_input (target_register_t dest, int maxlen, int block)
{
  char buf[4];
  int len, i;
  char d;

  buf[0] = 'I';
  buf[1] = '0';
  buf[2] = block ? '0' : '1';
  buf[3] = 0;
  __putpacket (buf);
  getpacket (remcomInBuffer);
  if (remcomInBuffer[0] != 'I')
    return -1;
  len = stubhex (remcomInBuffer[1]) * 16 + stubhex (remcomInBuffer[2]);
  for (i = 0; i < len; i++)
    {
      d = stubhex (remcomInBuffer[3 + i * 2]) * 16;
      d |=  stubhex (remcomInBuffer[3 + i * 2 + 1]);
      __write_mem_safe (&d, (void *)(dest + i), 1);
    }
  /* Write the trailing \0. */
  d = '\0';
  __write_mem_safe (&d, (void *)(dest + i), 1);
  return len;
}

void
__output_hex_value (target_register_t i)
{
  char buf[32], *ptr=buf+31;
  unsigned int x;

  *ptr = 0;
  for (x = 0; x < (sizeof (i) * 2); x++)
    {
      *(--ptr) = hexchars[i & 15];
      i = i >> 4;
    }
  while (*ptr)
    {
      putDebugChar (*(ptr++));
    }
}

/* Write the C-style string pointed to by STR to the GDB comm port. */
void
__putDebugStr (char *str)
{
  while (*str)
    {
      putDebugChar (*str);
      str++;
    }
}

/* Send STRING_LEN bytes of STR to GDB, using 'O' packets.
   STR is assumed to be in the program being debugged. */

int
__output_gdb_string (target_register_t str, int string_len)
{
  /* We will arbitrarily limit output packets to less than 400 bytes. */
  static char buf[400];
  int x;
  int len;

  if (string_len == 0)
    {
      /* We can't do strlen on a user pointer. */
      return -1;
    }

  len = string_len;
  while (len > 0)
    {
      int packetlen = ((len < 175) ? len : 175);
      buf[0] = 'O';
      for (x = 0; x < packetlen; x++) 
        {
          char c;

          __read_mem_safe (&c, (void *)(str + x), 1);
          buf[x*2+1] = hexchars[(c >> 4) & 0xf];
          buf[x*2+2] = hexchars[c % 16];
        }
      str += x;
      len -= x;
      buf[x*2+1] = 0;
      __putpacket (buf);
    }
  return string_len;
}

static void
do_nothing (void)
{
  /* mmmm */
}

static int
syscall_do_nothing (int junk)
{
  return 0;
}

/* Start the stub running. */
void
__switch_to_stub (void)
{
  __process_exception_vec = process_exception;
}

#if ! defined(BOARD_SPECIFIC_STUB_INIT)
void
initialize_stub (void)
{
  set_debug_traps ();
  /* FIXME: This function should be renamed to specifically init the
     hardware required by debug operations. If initHardware is implemented at
     all, it should be called before main ().
     */
  initHardware () ;
  /* This acks any stale packets , NOT an effective solution */
  putDebugChar ('+');
}
#endif

void
ungetDebugChar (int c)
{
  ungot_char = c;
}

void
__kill_program (int sigval)
{
  remcomOutBuffer[0] = 'X';
  remcomOutBuffer[1] = hexchars[(sigval >> 4) & 15];
  remcomOutBuffer[2] = hexchars[sigval & 15];
  remcomOutBuffer[3] = 0;
  __putpacket (remcomOutBuffer);
}

#define MAX_ARG_COUNT 20
#define MAX_ARGDATA 128

static char *program_argv [MAX_ARG_COUNT];
static int program_argc;
static int last_program_arg;
static char program_argstr [MAX_ARGDATA], *argptr;
static int args_initted = 0;

void
__free_program_args (void)
{
  last_program_arg = -1;
  program_argc = 0;
  program_argv [0] = NULL;
  argptr = program_argstr;
  args_initted = 1;
}

static char *
__add_program_arg (int argc, uint32 len)
{
  char *res;

  if (! args_initted)
    {
      __free_program_args ();
    }

  if ((argc >= (MAX_ARG_COUNT - 1))
      || ((argptr - program_argstr + len) > MAX_ARGDATA))
    {
      return NULL;
    }

  if (argc != last_program_arg)
    {
      if (argc >= program_argc)
        {
          program_argc = argc + 1;
          program_argv [program_argc] = NULL;
        }
      program_argv [argc] = argptr;
      last_program_arg = argc;
    }

  res = argptr;
  argptr += len;

  return res;
}

void
__set_program_args (int argc, char **argv)
{
  int x;

  __free_program_args ();
  if (argc)
    {
      for (x = 0; x < argc; x++)
        {
          uint32 len = strlen (argv[x])+1;
          char *s = __add_program_arg (x, len);

          if (s == NULL)
            return;

          memcpy (s, argv[x], len);
        }
    }
}

char **
__get_program_args (target_register_t argcPtr)
{
  if (!args_initted)
    {
      __free_program_args ();
    }
  __write_mem_safe ((char *) &program_argc, (void *)argcPtr, sizeof (program_argc));
  return program_argv;
}

/* Table used by the crc32 function to calcuate the checksum. */
static uint32 crc32_table[256];
static int tableInit = 0;

/* 
   Calculate a CRC-32 using LEN bytes of PTR. CRC is the initial CRC
   value.
   PTR is assumed to be a pointer in the user program. */

static uint32
crc32 (ptr, len, crc)
     unsigned char *ptr;
     int len;
     uint32 crc;
{
  if (! tableInit)
    {
      /* Initialize the CRC table and the decoding table. */
      uint32 i, j;
      uint32 c;

      tableInit = 1;
      for (i = 0; i < 256; i++)
        {
          for (c = i << 24, j = 8; j > 0; --j)
            c = c & 0x80000000 ? (c << 1) ^ 0x04c11db7 : (c << 1);
          crc32_table[i] = c;
        }
    }

  __mem_fault = 0;
  while (len--)
    {
      unsigned char ch;

      __read_mem_safe (&ch, (void *)ptr, 1);
      if (__mem_fault)
        {
          break;
        }
      crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ ch) & 255];
      ptr++;
    }
  return crc;
}

/* Handle the 'q' request */

static void
process_query (char *pkt)
{
  remcomOutBuffer[0] = '\0';
#ifdef __ECOS__
  if ('C' == pkt[0] &&
      'R' == pkt[1] &&
      'C' == pkt[2] &&
      ':' == pkt[3])
#else  // __ECOS__
  if (strncmp (pkt, "CRC:", 4) == 0)
#endif // __ECOS__
    {
      target_register_t startmem;
      target_register_t length;
      uint32 our_crc;

      pkt += 4;
      if (__hexToInt (&pkt, &startmem)
          && *(pkt++) == ','
          && __hexToInt (&pkt, &length))
        {
          our_crc = crc32 ((unsigned char *) startmem, length, 0xffffffff);
          if (__mem_fault)
            {
              strcpy (remcomOutBuffer, "E01");
            }
          else
            {
              int numb = __intToHex (remcomOutBuffer + 1, our_crc, 32);
              remcomOutBuffer[0] = 'C';
              remcomOutBuffer[numb + 1] = 0;
            }
        }
      return;
    }
  else
    {
      char ch ;
      char * subpkt ;
      ch = *pkt ;
      subpkt = pkt + 1 ;
      switch (ch)
        {
        case 'L' : /* threadlistquery */
          STUB_PKT_GETTHREADLIST (subpkt, remcomOutBuffer, 300);
          break ;
        case 'P' : /* Thread or process information request */
          STUB_PKT_GETTHREADINFO (subpkt, remcomOutBuffer, 300);
          break ;
        case 'C' : /* current thread query */
          STUB_PKT_CURRTHREAD(subpkt, remcomOutBuffer, sizeof(remcomOutBuffer));
          break;
        default:
          __process_target_query (pkt, remcomOutBuffer, 300);
          break ;
        }
    }
}

/* Handle the 'Q' request */

static void
process_set (char *pkt)
{
  char ch ;
  ch = *pkt ;
  
  switch (ch)
    {
    case 'p' : /* Set current process or thread */
      /* reserve the packet id even if support is not present */
      /* Dont strip the 'p' off the header, there are several variations of
         this packet */
      STUB_PKT_CHANGETHREAD (pkt, remcomOutBuffer, 300) ;
      break ;
    default:
      __process_target_set (pkt, remcomOutBuffer, 300);
      break ;
    }
}
#endif // CYGDBG_HAL_DEBUG_GDB_INCLUDE_STUBS
