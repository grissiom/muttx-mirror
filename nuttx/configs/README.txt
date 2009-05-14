Board-Specific Configurations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Table of Contents
^^^^^^^^^^^^^^^^^

  o Board-Specific Configurations
  o Summary of Files
  o Supported Architectures
  o Configuring NuttX

Board-Specific Configurations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The NuttX configuration consists of:

o Processor architecture specific files.  These are the files contained
  in the arch/<arch-name>/ directory.

o Chip/SoC specific files.  Each processor processor architecture
  is embedded in chip or System-on-a-Chip (SoC) architecture.  The
  full chip architecture includes the processor architecture plus
  chip-specific interrupt logic, general purpose I/O (GIO) logic, and
  specialized, internal peripherals (such as UARTs, USB, etc.).

  These chip-specific files are contained within chip-specific
  sub-directories in the arch/<arch-name>/ directory and are selected
  via the CONFIG_ARCH_name selection

o Board specific files.  In order to be usable, the chip must be
  contained in a board environment.  The board configuration defines
  additional properties of the board including such things as
  peripheral LEDs, external peripherals (such as network, USB, etc.).

  These board-specific configuration files can be found in the
  configs/<board-name>/ sub-directories and are discussed in this
  README.  Additional configuration information maybe available in
  board-specific configs/<board-name>/README.txt files.

The configs/ subdirectory contains configuration data for each board.  These
board-specific configurations plus the architecture-specific configurations in
the arch/ subdirectory completely define a customized port of NuttX.

Directory Structure
^^^^^^^^^^^^^^^^^^^

The configs directory contains board specific configurationlogic.  Each
board must provide a subdirectory <board-name> under configs/ with the
following characteristics:


	<board-name>
	|-- include/
	|   `-- (board-specific header files)
	|-- src/
	|   |-- Makefile
	|   `-- (board-specific source files)
        |-- <config1-dir>
	|   |-- Make.defs
	|   |-- defconfig
	|   `-- setenv.sh
        |-- <config2-dir>
	|   |-- Make.defs
	|   |-- defconfig
	|   `-- setenv.sh
	...
Summary of Files
^^^^^^^^^^^^^^^^

include/ -- This directory contains board specific header files.  This
  directory will be linked as include/arch/board at configuration time and
  can be included via '#include <arch/board/header.h>'.  These header file
  can only be included by files in arch/<arch-name>include/ and
  arch/<arch-name>/src

src/ -- This directory contains board specific drivers.  This
  directory will be linked as arch/<arch-name>/src/board at configuration
  time and will be integrated into the build system.

src/Makefile -- This makefile will be invoked to build the board specific
  drivers.  It must support the following targets:  libext$(LIBEXT), clean,
  and distclean.

A board may have various different configurations using these common source
files.  Each board configuration is described by three files:  Make.defs,
defconfig, and setenv.sh.  Typically, each set of configuration files is
retained in a separate configuration sub-directory (<config1-dir>,
<config2-dir>, .. in the above diagram).

Make.defs -- This makefile fragment provides architecture and
  tool-specific build options.  It will be included by all other
  makefiles in the build (once it is installed).  This make fragment
  should define:

	Tools: CC, LD, AR, NM, OBJCOPY, OBJDUMP
	Tool options: CFLAGS, LDFLAGS
	COMPILE, ASSEMBLE, ARCHIVE, CLEAN, and MKDEP macros

  When this makefile fragment runs, it will be passed TOPDIR which
  is the path to the root directory of the build.  This makefile
  fragment may include ${TOPDIR}/.config to perform configuration
  specific settings.  For example, the CFLAGS will most likely be
  different if CONFIG_DEBUG=y.

defconfig -- This is a configuration file similar to the Linux
  configuration file.  In contains variable/value pairs like:

	CONFIG_VARIABLE=value

  This configuration file will be used at build time:

    (1) as a makefile fragment included in other makefiles, and
    (2) to generate include/nuttx/config.h which is included by
        most C files in the system.

  The following variables are recognized by the build (you may
  also include architecture/board-specific settings).

	Architecture selection:

		CONFIG_ARCH - Identifies the arch/ subdirectory
		CONFIG_ARCH_name - For use in C code
		CONFIG_ARCH_CHIP - Identifies the arch/*/chip subdirectory
		CONFIG_ARCH_CHIP_name - For use in C code
		CONFIG_ARCH_BOARD - Identifies the configs subdirectory and
		   hence, the board that supports the particular chip or SoC.
		CONFIG_ARCH_BOARD_name - For use in C code
		CONFIG_ENDIAN_BIG - define if big endian (default is little
		   endian)
		CONFIG_ARCH_NOINTC - define if the architecture does not
		  support an interrupt controller or otherwise cannot support
		  APIs like up_enable_irq() and up_disable_irq().
		CONFIG_ARCH_IRQPRIO
		  Define if the architecture suports prioritizaton of interrupts
		  and the up_prioritize_irq() API.

	Some architectures require a description of the RAM configuration:

		CONFIG_DRAM_SIZE - Describes the installed DRAM.
		CONFIG_DRAM_START - The start address of DRAM (physical)
		CONFIG_DRAM_VSTART - The start address of DRAM (virtual)


	General build options

		CONFIG_RRLOAD_BINARY - make the rrload binary format used with
		  BSPs from www.ridgerun.com using the tools/mkimage.sh script.
		CONFIG_INTELHEX_BINARY - make the Intel HEX binary format
		  used with many different loaders using the GNU objcopy program
		  Should not be selected if you are not using the GNU toolchain.
		CONFIG_MOTOROLA_SREC - make the Motorola S-Record binary format
		  used with many different loaders using the GNU objcopy program
		  Should not be selected if you are not using the GNU toolchain.
		CONFIG_RAW_BINARY - make a raw binary format file used with many
		  different loaders using the GNU objcopy program.  This option
		  should not be selected if you are not using the GNU toolchain.
		CONFIG_HAVE_LIBM - toolchain supports libm.a
		CONFIG_HAVE_CXX - toolchain supports C++ and CXX, CXXFLAGS, and
		  COMPILEXX have been defined in the configuratins Make.defs
		  file.

	General OS setup

		CONFIG_EXAMPLE - identifies the subdirectory in examples
		  that will be used in the build
		CONFIG_DEBUG - enables built-in debug options
		CONFIG_DEBUG_VERBOSE - enables verbose debug output
		CONFIG_DEBUG_SCHED - enable OS debug output (disabled by
		  default)
		CONFIG_DEBUG_MM - enable memory management debug output
		  (disabled by default)
		CONFIG_DEBUG_NET - enable network debug output (disabled
		  by default)
		CONFIG_DEBUG_FS - enable filesystem debug output (disabled
		  by default)
		CONFIG_DEBUG_LIB - enable C library debug output (disabled
		  by default)
		CONFIG_ARCH_LOWPUTC - architecture supports low-level, boot
		  time console output
		CONFIG_MM_REGIONS - If the architecture includes multiple
		  regions of memory to allocate from, this specifies the
		  number of memory regions that the memory manager must
		  handle and enables the API mm_addregion(start, end);
		CONFIG_TICKS_PER_MSEC - The default system timer is 100Hz
		  or TICKS_PER_MSEC=10.  This setting may be defined to
		  inform NuttX that the processor hardware is providing
		  system timer interrupts at some interrupt interval other
		  than 10 msec.
		CONFIG_RR_INTERVAL - The round robin timeslice will be set
		  this number of milliseconds;  Round robin scheduling can
		  be disabled by setting this value to zero.
		CONFIG_SCHED_INSTRUMENTATION - enables instrumentation in 
		  scheduler to monitor system performance
		CONFIG_TASK_NAME_SIZE - Specifies that maximum size of a
		  task name to save in the TCB.  Useful if scheduler
		  instrumentation is selected.  Set to zero to disable.
		CONFIG_START_YEAR, CONFIG_START_MONTH, CONFIG_START_DAY -
		  Used to initialize the internal time logic.
		CONFIG_JULIAN_TIME - Enables Julian time conversions
		CONFIG_DEV_CONSOLE - Set if architecture-specific logic
		  provides /dev/console.  Enables stdout, stderr, stdin.
		CONFIG_MUTEX_TYPES - Set to enable support for recursive and
		  errorcheck mutexes.  Enables pthread_mutexattr_settype().
		CONFIG_PRIORITY_INHERITANCE - Set to enable support for
		  priority inheritance on mutexes and semaphores.
		CONFIG_SEM_PREALLOCHOLDERS: This setting is only used if priority
		  inheritance is enabled.  It defines the maximum number of
		  different threads (minus one) that can take counts on a
		  semaphore with priority inheritance support.  This may be 
		  set to zero if priority inheritance is disabled OR if you
		  are only using semaphores as mutexes (only one holder) OR
		  if no more than two threads participate using a counting
		  semaphore.
		CONFIG_SEM_NNESTPRIO.  If priority inheritance is enabled,
		  then this setting is the maximum number of higher priority
		  threads (minus 1) than can be waiting for another thread
		  to release a count on a semaphore.  This value may be set
		  to zero if no more than one thread is expected to wait for
		  a semaphore.

	The following can be used to disable categories of APIs supported
	by the OS.  If the compiler supports weak functions, then it
	should not be necessary to disable functions unless you want to
	restrict usage of those APIs.

	There are certain dependency relationships in these features.

	o mq_notify logic depends on signals to awaken tasks
	  waiting for queues to become full or empty.
	o pthread_condtimedwait() depends on signals to wake
	  up waiting tasks.

		CONFIG_DISABLE_CLOCK, CONFIG_DISABLE_POSIX_TIMERS, CONFIG_DISABLE_PTHREAD.
		CONFIG_DISABLE_SIGNALS, CONFIG_DISABLE_MQUEUE, CONFIG_DISABLE_MOUNTPOUNT,
		CONFIG_DISABLE_ENVIRON, CONFIG_DISABLE_POLL


	Misc libc settings

		CONFIG_NOPRINTF_FIELDWIDTH - sprintf-related logic is a
		   little smaller if we do not support fieldwidthes

	Allow for architecture optimized implementations

		The architecture can provide optimized versions of the
		following to improve system performance

		CONFIG_ARCH_MEMCPY, CONFIG_ARCH_MEMCMP, CONFIG_ARCH_MEMMOVE
		CONFIG_ARCH_MEMSET, CONFIG_ARCH_STRCMP, CONFIG_ARCH_STRCPY
		CONFIG_ARCH_STRNCPY, CONFIG_ARCH_STRLEN, CONFIG_ARCH_BZERO
		CONFIG_ARCH_KMALLOC, CONFIG_ARCH_KZMALLOC, CONFIG_ARCH_KFREE

	Sizes of configurable things (0 disables)

		CONFIG_MAX_TASKS - The maximum number of simultaneously
		  active tasks.  This value must be a power of two.
		CONFIG_NPTHREAD_KEYS - The number of items of thread-
		  specific data that can be retained
		CONFIG_NFILE_DESCRIPTORS - The maximum number of file
		  descriptors (one for each open)
		CONFIG_NFILE_STREAMS - The maximum number of streams that
		  can be fopen'ed
		CONFIG_NAME_MAX - The maximum size of a file name.
		CONFIG_STDIO_BUFFER_SIZE - Size of the buffer to allocate
		  on fopen. (Only if CONFIG_NFILE_STREAMS > 0)
		CONFIG_NUNGET_CHARS - Number of characters that can be
		  buffered by ungetc() (Only if CONFIG_NFILE_STREAMS > 0)
		CONFIG_PREALLOC_MQ_MSGS - The number of pre-allocated message
		  structures.  The system manages a pool of preallocated
		  message structures to minimize dynamic allocations
		CONFIG_MQ_MAXMSGSIZE - Message structures are allocated with
		  a fixed payload size given by this settin (does not include
		  other message structure overhead.
		CONFIG_PREALLOC_WDOGS - The number of pre-allocated watchdog
		  structures.  The system manages a pool of preallocated
		  watchdog structures to minimize dynamic allocations
		CONFIG_DEV_PIPE_SIZE - Size, in bytes, of the buffer to allocated
		  for pipe and FIFO support

	Filesystem configuration
		CONFIG_FS_FAT - Enable FAT filesystem support
		CONFIG_FAT_SECTORSIZE - Max supported sector size
		CONFIG_FS_ROMFS - Enable ROMFS filesystem support

	SPI-based MMC/SD driver
		CONFIG_MMCSD_NSLOTS - Number of MMC/SD slots supported by the
		  driver. Default is one.
		CONFIG_MMCSD_READONLY -  Provide read-only access.  Default is
		  Read/Write

	TCP/IP and UDP support via uIP
		CONFIG_NET - Enable or disable all network features
		CONFIG_NET_IPv6 - Build in support for IPv6
		CONFIG_NSOCKET_DESCRIPTORS - Maximum number of socket descriptors
                  per task/thread.
		CONFIG_NET_NACTIVESOCKETS - Maximum number of concurrent socket
		  operations (recv, send, etc.).  Default: CONFIG_NET_TCP_CONNS+CONFIG_NET_UDP_CONNS
		CONFIG_NET_SOCKOPTS - Enable or disable support for socket options

		CONFIG_NET_BUFSIZE - uIP buffer size
		CONFIG_NET_TCPURGDATA - Determines if support for TCP urgent data
		  notification should be compiled in. Urgent data (out-of-band data)
		  is a rarely used TCP feature that is very seldom would be required.
		CONFIG_NET_TCP - TCP support on or off
		CONFIG_NET_TCP_CONNS - Maximum number of TCP connections (all tasks)
		CONFIG_NET_MAX_LISTENPORTS - Maximum number of listening TCP ports (all tasks)
		CONFIG_NET_TCP_READAHEAD_BUFSIZE - Size of TCP read-ahead buffers
		CONFIG_NET_NTCP_READAHEAD_BUFFERS - Number of TCP read-ahead buffers
		  (may be zero)
		CONFIG_NET_TCPBACKLOG - Incoming connections pend in a backlog until
		  accept() is called. The size of the backlog is selected when listen()
		  is called.
		CONFIG_NET_UDP - UDP support on or off
		CONFIG_NET_UDP_CHECKSUMS - UDP checksums on or off
		CONFIG_NET_UDP_CONNS - The maximum amount of concurrent UDP
		  connections
		CONFIG_NET_ICMP - Enable minimal ICMP support. Includes built-in support
		  for sending replies to received ECHO (ping) requests.
		CONFIG_NET_ICMP_PING - Provide interfaces to support application level
		  support for sending ECHO (ping) requests and associating ECHO
		  replies.
		CONFIG_NET_PINGADDRCONF - Use "ping" packet for setting IP address
		CONFIG_NET_STATISTICS - uIP statistics on or off
		CONFIG_NET_RECEIVE_WINDOW - The size of the advertised receiver's
		  window
		CONFIG_NET_ARPTAB_SIZE - The size of the ARP table
		CONFIG_NET_BROADCAST - Incoming UDP broadcast support
		CONFIG_NET_MULTICAST - Outgoing multi-cast address support
		CONFIG_NET_LLH_LEN - The link level header length
		CONFIG_NET_FWCACHE_SIZE - number of packets to remember when
		  looking for duplicates

	UIP Network Utilities
		CONFIG_NET_DHCP_LIGHT - Reduces size of DHCP
		CONFIG_NET_RESOLV_ENTRIES - Number of resolver entries

	USB device controller driver
		CONFIG_USBDEV - Enables USB device support
		CONFIG_USBDEV_ISOCHRONOUS - Build in extra support for isochronous
		  endpoints
		CONFIG_USBDEV_DUALSPEED -Hardware handles high and full speed
		  operation (USB 2.0)
		CONFIG_USBDEV_SELFPOWERED - Will cause USB features to indicate
		  that the device is self-powered
		CONFIG_USBDEV_MAXPOWER - Maximum power consumption in mA
		CONFIG_USBDEV_TRACE - Enables USB tracing for debug
		CONFIG_USBDEV_TRACE_NRECORDS - Number of trace entries to remember

	USB serial device class driver
		CONFIG_USBSER
		  Enable compilation of the USB serial driver
		CONFIG_USBSER_EPINTIN
		  The logical 7-bit address of a hardware endpoint that supports
		  interrupt IN operation
		CONFIG_USBSER_EPBULKOUT
		  The logical 7-bit address of a hardware endpoint that supports
		  bulk OUT operation
		CONFIG_USBSER_EPBULKIN
		  The logical 7-bit address of a hardware endpoint that supports
		  bulk IN operation
		CONFIG_USBSER_NWRREQS and CONFIG_USBSER_NRDREQS
		  The number of write/read requests that can be in flight
		CONFIG_USBSER_VENDORID and CONFIG_USBSER_VENDORSTR
		  The vendor ID code/string
		CONFIG_USBSER_PRODUCTID and CONFIG_USBSER_PRODUCTSTR
		  The product ID code/string
		CONFIG_USBSER_RXBUFSIZE and CONFIG_USBSER_TXBUFSIZE
		  Size of the serial receive/transmit buffers

	USB Storage Device Configuration
		CONFIG_USBSTRG
		  Enable compilation of the USB storage driver
		CONFIG_USBSTRG_EP0MAXPACKET
		  Max packet size for endpoint 0
		CONFIG_USBSTRGEPBULKOUT and CONFIG_USBSTRG_EPBULKIN
		  The logical 7-bit address of a hardware endpoints that support
		  bulk OUT and IN operations
		CONFIG_USBSTRG_NWRREQS and CONFIG_USBSTRG_NRDREQS
		  The number of write/read requests that can be in flight
		CONFIG_USBSTRG_BULKINREQLEN and CONFIG_USBSTRG_BULKOUTREQLEN
		  The size of the buffer in each write/read request.  This
		  value needs to be at least as large as the endpoint
		  maxpacket and ideally as large as a block device sector.
		CONFIG_USBSTRG_VENDORID and CONFIG_USBSTRG_VENDORSTR
		  The vendor ID code/string
		CONFIG_USBSTRG_PRODUCTID and CONFIG_USBSTRG_PRODUCTSTR
		  The product ID code/string
		CONFIG_USBSTRG_REMOVABLE
		  Select if the media is removable

	Graphics related configuration settings

		CONFIG_NX
		  Enables overall support for graphics library and NX
		CONFIG_NX_MULTIUSER
		  Configures NX in multi-user mode
		CONFIG_NX_NPLANES
		  Some YUV color formats requires support for multiple planes,
		  one for each color component.  Unless you have such special
		  hardware, this value should be undefined or set to 1.
		CONFIG_NX_DISABLE_1BPP, CONFIG_NX_DISABLE_2BPP,
		CONFIG_NX_DISABLE_4BPP, CONFIG_NX_DISABLE_8BPP,
		CONFIG_NX_DISABLE_16BPP, CONFIG_NX_DISABLE_24BPP, and
		CONFIG_NX_DISABLE_32BPP
		  NX supports a variety of pixel depths.  You can save some
		  memory by disabling support for unused color depths.
		CONFIG_NX_PACKEDMSFIRST
		  If a pixel depth of less than 8-bits is used, then NX needs
		  to know if the pixels pack from the MS to LS or from LS to MS
		CONFIG_NX_MOUSE
		  Build in support for mouse input.
		CONFIG_NX_KBD
		  Build in support of keypad/keyboard input.
		CONFIG_NXTK_BORDERWIDTH
		  Specifies with with of the border (in pixels) used with
		  framed windows.   The default is 4.
		CONFIG_NXTK_BORDERCOLOR1 and CONFIG_NXTK_BORDERCOLOR2
		  Specify the colors of the border used with framed windows.
		  CONFIG_NXTK_BORDERCOLOR2 is the shadow side color and so
		  is normally darker.  The default is medium and dark grey,
		  respectively
		CONFIG_NXTK_AUTORAISE
		  If set, a window will be raised to the top if the mouse position
		  is over a visible portion of the window.  Default: A mouse
		  button must be clicked over a visible portion of the window.
		CONFIG_NXFONTS_CHARBITS
		  The number of bits in the character set.  Current options are
		  only 7 and 8.  The default is 7.
		CONFIG_NXFONT_SANS
		  At present, there is only one font.  But if there were were more,
		  then this option would select the sans serif font.

	NX Multi-user only options:

		CONFIG_NX_BLOCKING
		  Open the client message queues in blocking mode.  In this case,
		  nx_eventhandler() will never return.
		CONFIG_NX_MXSERVERMSGS and CONFIG_NX_MXCLIENTMSGS
		  Specifies the maximum number of messages that can fit in
		  the message queues.  No additional resources are allocated, but
		  this can be set to prevent flooding of the client or server with
		  too many messages (CONFIG_PREALLOC_MQ_MSGS controls how many
		  messages are pre-allocated).

	Stack and heap information

		CONFIG_BOOT_RUNFROMFLASH - Some configurations support XIP
		  operation from FLASH but must copy initialized .data sections to RAM.
		CONFIG_BOOT_COPYTORAM -  Some configurations boot in FLASH
		  but copy themselves entirely into RAM for better performance.
		CONFIG_STACK_POINTER - The initial stack pointer
		CONFIG_IDLETHREAD_STACKSIZE - The size of the initial stack.
		  This is the thread that (1) performs the inital boot of the system up
		  to the point where user_start() is spawned, and (2) there after is the
		  IDLE thread that executes only when there is no other thread ready to
		  run.
		CONFIG_USERMAIN_STACKSIZE - The size of the stack to allocate
		  for the main user thread that begins at the user_start() entry point.
		CONFIG_PTHREAD_STACK_MIN - Minimum pthread stack size
		CONFIG_PTHREAD_STACK_DEFAULT - Default pthread stack size
		CONFIG_HEAP_BASE - The beginning of the heap
		CONFIG_HEAP_SIZE - The size of the heap

setenv.sh -- This is a script that you can include that will be installed at
  the toplevel of the directory structure and can be sourced to set any
  necessary environment variables.

Supported Boards
^^^^^^^^^^^^^^^^

configs/c5471evm
    This is a port to the Spectrum Digital C5471 evaluation board.  The
    TMS320C5471 is a dual core processor from TI with an ARM7TDMI general
    purpose processor and a c54 DSP.  It is also known as TMS320DA180 or just DA180. 
    NuttX runs on the ARM core and is built with a GNU arm-elf toolchain*.
    This port is complete, verified, and included in the NuttX release.

configs/eagle100
    Micromint Eagle-100 Development board.  This board is based on the 
    an ARM Cortex-M3 MCU, the Luminary LM3S6918. This OS is built with the
    arm-elf toolchain*.  STATUS:  This port is currently under development.

configs/ez80f0910200kitg
    ez80Acclaim! Microcontroller.  This port use the Zilog ez80f0910200kitg
    development kit, eZ80F091 part, and the Zilog ZDS-II Windows command line
    tools.  The development environment is Cygwin under WinXP.

configs/ez80f0910200zco
    ez80Acclaim! Microcontroller.  This port use the Zilog ez80f0910200zco
    development kit, eZ80F091 part, and the Zilog ZDS-II Windows command line
    tools.  The development environment is Cygwin under WinXP.

configs/m68322evb
    This is a work in progress for the venerable m68322evb board from
    Motorola. This OS is also built with the arm-elf toolchain*.

configs/mcu123-lpc214x
    This port is for the NXP LPC2148 as provided on the mcu123.com
    lpc214x development board. This OS is also built with the arm-elf
    toolchain*.  The port supports serial, timer0, spi, and usb.

configs/mx1ads
    This is a port to the Motorola MX1ADS development board.  That board
    is based on the Freescale i.MX1 processor.  The i.MX1 is an ARM920T.
    STATUS:  This port is nearly code complete but still under development
    (work is stalled until I devote time to the Micromint Eagle-100)

configs/ntosd-dm320
    This port uses the Neuros OSD with a GNU arm-elf toolchain*:
    see http://wiki.neurostechnology.com/index.php/Developer_Welcome .
    NuttX operates on the ARM9EJS of this dual core processor.
    STATUS: This port is code complete, verified, and included in the
    NuttX 0.2.1 release.

configs/olimex-strp711
    This port uses the Olimex STR-P711 board arm-elf toolchain* under Linux or Cygwin.
    See the http://www.olimex.com/dev/str-p711.html" for futher information.
    STATUS: Coding for the basic port -- serial console and system timer -- is complete
    but untested to problems I am having using OpenOCD with a wiggler clone JTAG.

configs/pjrc-8051
    8051 Microcontroller.  This port uses the PJRC 87C52 development system
    and the SDCC toolchain.   This port is not quite ready for prime time.

configs/sim
    A user-mode port of NuttX to the x86 Linux platform is available.
    The purpose of this port is primarily to support OS feature development.
    This port does not support interrupts or a real timer (and hence no
    round robin scheduler)  Otherwise, it is complete.

    NOTE: This target will not run on Cygwin probably for many reasons but
    first off because it uses some of the same symbols as does cygwin.dll.

configs/skp16c26
    Renesas M16C processor on the Renesas SKP16C26 StarterKit.  This port
    uses the GNU m32c toolchain.

configs/us7032evb1
    This is a port of the Hitachi SH-1 on the Hitachi SH-1/US7032EVB1 board.
    STATUS:  Work has just began on this port.

configs/xtrs
    TRS80 Model 3.  This port uses a vintage computer based on the Z80.
    An emulator for this computer is available to run TRS80 programs on a 
    linux platform (http://www.tim-mann.org/xtrs.html).

configs/z16f2800100zcog
    z16f Microcontroller.  This port use the Zilog z16f2800100zcog
    development kit and the Zilog ZDS-II Windows command line tools.  The
    development environment is Cygwin under WinXP.

configs/z80sim
    z80 Microcontroller.  This port uses a Z80 instruction set simulator.
    That simulator can be found in the NuttX CVS at
    http://nuttx.cvs.sourceforge.net/nuttx/misc/sims/z80sim.
    This port also uses the SDCC toolchain (http://sdcc.sourceforge.net/")
    (verified with version 2.6.0).

configs/z8encore000zco
    z8Encore! Microcontroller.  This port use the Zilog z8encore000zco
    development kit, Z8F6403 part, and the Zilog ZDS-II Windows command line
    tools.  The development environment is Cygwin under WinXP.

configs/z8f64200100kit
    z8Encore! Microcontroller.  This port use the Zilog z8f64200100kit
    development kit, Z8F6423 part, and the Zilog ZDS-II Windows command line
    tools.  The development environment is Cygwin under WinXP.

Other ports for the for the TI TMS320DM270, M683222 and for MIPS are in various
states of progress

Configuring NuttX
^^^^^^^^^^^^^^^^^

Configuring NuttX requires only copying

  configs/<board-name>/<config-dir>/Make.def to ${TOPDIR}/Make.defs
  configs/<board-name>/<config-dir>/setenv.sh to ${TOPDIR}/setenv.sh
  configs/<board-name>/<config-dir>/defconfig to ${TOPDIR}/.config

tools/configure.sh
  There is a script that automates these steps.  The following steps will
  accomplish the same configuration:

  cd tools
  ./configure.sh <board-name>/<config-dir>
