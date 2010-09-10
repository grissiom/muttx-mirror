README
^^^^^^

README for NuttX port to the mbed.org LPC1768 board (http://mbed.org/)

Contents
^^^^^^^^

  Development Environment
  GNU Toolchain Options
  IDEs
  NuttX buildroot Toolchain
  USB Device Controller Functions
  mbed Configuration Options
  Configurations

Development Environment
^^^^^^^^^^^^^^^^^^^^^^^

  Either Linux or Cygwin on Windows can be used for the development environment.
  The source has been built only using the GNU toolchain (see below).  Other
  toolchains will likely cause problems. Testing was performed using the Cygwin
  environment.

GNU Toolchain Options
^^^^^^^^^^^^^^^^^^^^^

  The NuttX make system has been modified to support the following different
  toolchain options.

  1. The CodeSourcery GNU toolchain,
  2. The devkitARM GNU toolchain,
  3. The NuttX buildroot Toolchain (see below).

  All testing has been conducted using the NuttX buildroot toolchain.  However,
  the make system is setup to default to use the devkitARM toolchain.  To use
  the CodeSourcery or devkitARM toolchain, you simply need add one of the
  following configuration options to your .config (or defconfig) file:

    CONFIG_LPC17_CODESOURCERYW=y   : CodeSourcery under Windows
    CONFIG_LPC17_CODESOURCERYL=y   : CodeSourcery under Linux
    CONFIG_LPC17_DEVKITARM=y       : devkitARM under Windows
    CONFIG_LPC17_BUILDROOT=y       : NuttX buildroot under Linux or Cygwin (default)

  If you are not using CONFIG_LPC17_BUILDROOT, then you may also have to modify
  the PATH in the setenv.h file if your make cannot find the tools.

  NOTE: the CodeSourcery (for Windows)and devkitARM are Windows native toolchains.
  The CodeSourcey (for Linux) and NuttX buildroot toolchains are Cygwin and/or
  Linux native toolchains. There are several limitations to using a Windows based
  toolchain in a Cygwin environment.  The three biggest are:

  1. The Windows toolchain cannot follow Cygwin paths.  Path conversions are
     performed automatically in the Cygwin makefiles using the 'cygpath' utility
     but you might easily find some new path problems.  If so, check out 'cygpath -w'

  2. Windows toolchains cannot follow Cygwin symbolic links.  Many symbolic links
     are used in Nuttx (e.g., include/arch).  The make system works around these
     problems for the Windows tools by copying directories instead of linking them.
     But this can also cause some confusion for you:  For example, you may edit
     a file in a "linked" directory and find that your changes had not effect.
     That is because you are building the copy of the file in the "fake" symbolic
     directory.  If you use a Windows toolchain, you should get in the habit of
     making like this:

       make clean_context all

     An alias in your .bashrc file might make that less painful.

  3. Dependencies are not made when using Windows versions of the GCC.  This is
     because the dependencies are generated using Windows pathes which do not
     work with the Cygwin make.

     Support has been added for making dependencies with the windows-native toolchains.
     That support can be enabled by modifying your Make.defs file as follows:

    -  MKDEP                = $(TOPDIR)/tools/mknulldeps.sh
    +  MKDEP                = $(TOPDIR)/tools/mkdeps.sh --winpaths "$(TOPDIR)"

     If you have problems with the dependency build (for example, if you are not
     building on C:), then you may need to modify tools/mkdeps.sh

  NOTE 1: The CodeSourcery toolchain (2009q1) does not work with default optimization
  level of -Os (See Make.defs).  It will work with -O0, -O1, or -O2, but not with
  -Os.

  NOTE 2: The devkitARM toolchain includes a version of MSYS make.  Make sure that
  the paths to Cygwin's /bin and /usr/bin directories appear BEFORE the devkitARM
  path or will get the wrong version of make.

IDEs
^^^^

  NuttX is built using command-line make.  It can be used with an IDE, but some
  effort will be required to create the project (There is a simple RIDE project
  in the RIDE subdirectory).
  
  Makefile Build
  --------------
  Under Eclipse, it is pretty easy to set up an "empty makefile project" and
  simply use the NuttX makefile to build the system.  That is almost for free
  under Linux.  Under Windows, you will need to set up the "Cygwin GCC" empty
  makefile project in order to work with Windows (Google for "Eclipse Cygwin" -
  there is a lot of help on the internet).

  Native Build
  ------------
  Here are a few tips before you start that effort:

  1) Select the toolchain that you will be using in your .config file
  2) Start the NuttX build at least one time from the Cygwin command line
     before trying to create your project.  This is necessary to create
     certain auto-generated files and directories that will be needed.
  3) Set up include pathes:  You will need include/, arch/arm/src/lpc17xx,
     arch/arm/src/common, arch/arm/src/cortexm3, and sched/.
  4) All assembly files need to have the definition option -D __ASSEMBLY__
     on the command line.

  Startup files will probably cause you some headaches.  The NuttX startup file
  is arch/arm/src/lpc17x/lpc17_vectors.S.

NuttX buildroot Toolchain
^^^^^^^^^^^^^^^^^^^^^^^^^

  A GNU GCC-based toolchain is assumed.  The files */setenv.sh should
  be modified to point to the correct path to the Cortex-M3 GCC toolchain (if
  different from the default in your PATH variable).

  If you have no Cortex-M3 toolchain, one can be downloaded from the NuttX
  SourceForge download site (https://sourceforge.net/project/showfiles.php?group_id=189573).
  This GNU toolchain builds and executes in the Linux or Cygwin environment.

  1. You must have already configured Nuttx in <some-dir>/nuttx.

     cd tools
     ./configure.sh mbed/<sub-dir>

  2. Download the latest buildroot package into <some-dir>

  3. unpack the buildroot tarball.  The resulting directory may
     have versioning information on it like buildroot-x.y.z.  If so,
     rename <some-dir>/buildroot-x.y.z to <some-dir>/buildroot.

  4. cd <some-dir>/buildroot

  5. cp configs/cortexm3-defconfig-4.3.3 .config

  6. make oldconfig

  7. make

  8. Edit setenv.h, if necessary, so that the PATH variable includes
     the path to the newly built binaries.

  See the file configs/README.txt in the buildroot source tree.  That has more
  detailed PLUS some special instructions that you will need to follow if you
  are building a Cortex-M3 toolchain for Cygwin under Windows.

  NOTE: This is an OABI toolchain.

mbed Configuration Options
^^^^^^^^^^^^^^^^^^^^^^^^^^

	CONFIG_ARCH - Identifies the arch/ subdirectory.  This should
	   be set to:

	   CONFIG_ARCH=arm

	CONFIG_ARCH_family - For use in C code:

	   CONFIG_ARCH_ARM=y

	CONFIG_ARCH_architecture - For use in C code:

	   CONFIG_ARCH_CORTEXM3=y

	CONFIG_ARCH_CHIP - Identifies the arch/*/chip subdirectory

	   CONFIG_ARCH_CHIP=lpc17xx

	CONFIG_ARCH_CHIP_name - For use in C code to identify the exact
	   chip:

	   CONFIG_ARCH_CHIP_LPC1768=y

	CONFIG_ARCH_BOARD - Identifies the configs subdirectory and
	   hence, the board that supports the particular chip or SoC.

	   CONFIG_ARCH_BOARD=mbed (for the mbed.org board)

	CONFIG_ARCH_BOARD_name - For use in C code

	   CONFIG_ARCH_BOARD_MBED=y

	CONFIG_ARCH_LOOPSPERMSEC - Must be calibrated for correct operation
	   of delay loops

	CONFIG_ENDIAN_BIG - define if big endian (default is little
	   endian)

	CONFIG_DRAM_SIZE - Describes the installed DRAM (CPU SRAM in this case):

	   CONFIG_DRAM_SIZE=(32*1024) (32Kb)

	   There is an additional 32Kb of SRAM in AHB SRAM banks 0 and 1.

	CONFIG_DRAM_START - The start address of installed DRAM

	   CONFIG_DRAM_START=0x10000000

	CONFIG_DRAM_END - Last address+1 of installed RAM

	   CONFIG_DRAM_END=(CONFIG_DRAM_START+CONFIG_DRAM_SIZE)

	CONFIG_ARCH_IRQPRIO - The LPC17xx supports interrupt prioritization

	   CONFIG_ARCH_IRQPRIO=y

	CONFIG_ARCH_LEDS - Use LEDs to show state. Unique to boards that
	   have LEDs

	CONFIG_ARCH_INTERRUPTSTACK - This architecture supports an interrupt
	   stack. If defined, this symbol is the size of the interrupt
	    stack in bytes.  If not defined, the user task stacks will be
	  used during interrupt handling.

	CONFIG_ARCH_STACKDUMP - Do stack dumps after assertions

	CONFIG_ARCH_LEDS -  Use LEDs to show state. Unique to board architecture.

	CONFIG_ARCH_CALIBRATION - Enables some build in instrumentation that
	   cause a 100 second delay during boot-up.  This 100 second delay
	   serves no purpose other than it allows you to calibratre
	   CONFIG_ARCH_LOOPSPERMSEC.  You simply use a stop watch to measure
	   the 100 second delay then adjust CONFIG_ARCH_LOOPSPERMSEC until
	   the delay actually is 100 seconds.

	Individual subsystems can be enabled:
	  CONFIG_LPC17_MAINOSC=y
	  CONFIG_LPC17_PLL0=y
	  CONFIG_LPC17_PLL1=n
	  CONFIG_LPC17_ETHERNET=n
	  CONFIG_LPC17_USBHOST=n
	  CONFIG_LPC17_USBOTG=n
	  CONFIG_LPC17_USBDEV=n
	  CONFIG_LPC17_UART0=y
	  CONFIG_LPC17_UART1=n
	  CONFIG_LPC17_UART2=n
	  CONFIG_LPC17_UART3=n
	  CONFIG_LPC17_CAN1=n
	  CONFIG_LPC17_CAN2=n
	  CONFIG_LPC17_SPI=n
	  CONFIG_LPC17_SSP0=n
	  CONFIG_LPC17_SSP1=n
	  CONFIG_LPC17_I2C0=n
	  CONFIG_LPC17_I2C1=n
	  CONFIG_LPC17_I2S=n
	  CONFIG_LPC17_TMR0=n
	  CONFIG_LPC17_TMR1=n
	  CONFIG_LPC17_TMR2=n
	  CONFIG_LPC17_TMR3=n
	  CONFIG_LPC17_RIT=n
	  CONFIG_LPC17_PWM=n
	  CONFIG_LPC17_MCPWM=n
	  CONFIG_LPC17_QEI=n
	  CONFIG_LPC17_RTC=n
	  CONFIG_LPC17_WDT=n
	  CONFIG_LPC17_ADC=n
	  CONFIG_LPC17_DAC=n
	  CONFIG_LPC17_GPDMA=n
	  CONFIG_LPC17_FLASH=n

  LPC17xx specific device driver settings

	CONFIG_UARTn_SERIAL_CONSOLE - selects the UARTn for the
	   console and ttys0 (default is the UART0).
	CONFIG_UARTn_RXBUFSIZE - Characters are buffered as received.
	   This specific the size of the receive buffer
	CONFIG_UARTn_TXBUFSIZE - Characters are buffered before
	   being sent.  This specific the size of the transmit buffer
	CONFIG_UARTn_BAUD - The configure BAUD of the UART.  Must be
	CONFIG_UARTn_BITS - The number of bits.  Must be either 7 or 8.
	CONFIG_UARTn_PARTIY - 0=no parity, 1=odd parity, 2=even parity
	CONFIG_UARTn_2STOP - Two stop bits

  LPC17xx USB Configuration

	CONFIG_LPC17_USBDEV_FRAME_INTERRUPT
	  Handle USB Start-Of-Frame events. 
	  Enable reading SOF from interrupt handler vs. simply reading on demand.
	  Probably a bad idea... Unless there is some issue with sampling the SOF
	  from hardware asynchronously.
	CONFIG_LPC17_USBDEV_EPFAST_INTERRUPT
	  Enable high priority interrupts.  I have no idea why you might want to
	  do that
	CONFIG_LPC17_USBDEV_NDMADESCRIPTORS
	  Number of DMA descriptors to allocate in SRAM.
	CONFIG_LPC17_USBDEV_DMA
	  Enable lpc17xx-specific DMA support

Configurations
^^^^^^^^^^^^^^

Each mbed configuration is maintained in a sudirectory and can be selected
as follow:

	cd tools
	./configure.sh mbed/<subdir>
	cd -
	. ./setenv.sh

Where <subdir> is one of the following:

  nsh:
    Configures the NuttShell (nsh) located at examples/nsh.  The
    Configuration enables only the serial  NSH interfaces.
