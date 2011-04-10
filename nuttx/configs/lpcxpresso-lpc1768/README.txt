README
^^^^^^

README for NuttX port to the Embedded Artists' LPCXpresso base board with
the LPCXpresso daughter board.

Contents
^^^^^^^^

  LCPXpresso LPC1768 Board
  Development Environment
  GNU Toolchain Options
  NuttX buildroot Toolchain
  Code Red IDE
  LEDs
  LPCXpresso Configuration Options
  Configurations

LCPXpresso LPC1768 Board
^^^^^^^^^^^^^^^^^^^^^^^^

  Pin Description                  Connector On Board       Base Board
  -------------------------------- --------- -------------- ---------------------

  P0[0]/RD1/TXD3/SDA1               J6-9     I2C E2PROM SDA TXD3/SDA1
  P0[1]/TD1/RXD3/SCL                J6-10                   RXD2/SCL1
  P0[2]/TXD0/AD0[7]                 J6-21    
  P0[3]/RXD0/AD0[6]                 J6-22    
  P0[4]/I2SRX-CLK/RD2/CAP2.0        J6-38                   CAN_RX2
  P0[5]/I2SRX-WS/TD2/CAP2.1         J6-39                   CAN_TX2
  P0[6]/I2SRX_SDA/SSEL1/MAT2[0]     J6-8                    SSEL1
  P0[7]/I2STX_CLK/SCK1/MAT2[1]      J6-7                    SCK1
  P0[8]/I2STX_WS/MISO1/MAT2[2]      J6-6                    MISO1
  P0[9]/I2STX_SDA/MOSI1/MAT2[3]     J6-5                    MOSI1
  P0[10]                            J6-40                   TXD2/SDA2
  P0[11]                            J6-41                   RXD2/SCL2
  P0[15]/TXD1/SCK0/SCK              J6-13                   TXD1/SCK0
  P0[16]/RXD1/SSEL0/SSEL            J6-14                   RXD1/SSEL0
  P0[17]/CTS1/MISO0/MISO            J6-12                   MISO0
  P0[18]/DCD1/MOSI0/MOSI            J6-11                   MOSI0
  P0[19]/DSR1/SDA1                  PAD17                   N/A
  P0[20]/DTR1/SCL1                  PAD18    I2C E2PROM SCL N/A
  P0[21]/RI1/MCIPWR/RD1             J6-23                  
  P0[22]/RTS1/TD1                   J6-24    LED            
  P0[23]/AD0[0]/I2SRX_CLK/CAP3[0]   J6-15                   AD0.0
  P0[24]/AD0[1]/I2SRX_WS/CAP3[1]    J6-16                   AD0.1
  P0[25]/AD0[2]/I2SRX_SDA/TXD3      J6-17                   AD0.2
  P0[26]/AD0[3]/AOUT/RXD3           J6-18                   AD0.3/AOUT
  P0[27]/SDA0/USB_SDA               J6-25                   
  P0[28]/SCL0                       J6-26                   
  P0[29]/USB_D+                     J6-37                   USB_D+
  P0[30]/USB_D-                     J6-36                   USB_D-

  P1[0]/ENET-TXD0                   J6-34?  TXD0            TX-(Ethernet PHY)
  P1[1]/ENET_TXD1                   J6-35?  TXD1            TX+(Ethernet PHY)
  P1[4]/ENET_TX_EN                          TXEN            N/A
  P1[8]/ENET_CRS                            CRS_DV/MODE2    N/A
  P1[9]/ENET_RXD0                   J6-32?  RXD0/MODE0      RD-(Ethernet PHY)
  P1[10]/ENET_RXD1                  J6-33?  RXD1/MODE1      RD+(Ethernet PHY)
  P1[14]/ENET_RX_ER                         RXER/PHYAD0     N/A
  P1[15]/ENET_REF_CLK                       REFCLK          N/A
  P1[16]/ENET_MDC                           MDC             N/A
  P1[17]/ENET_MDIO                          MDIO            N/A
  P1[18]/USB_UP_LED/PWM1[1]/CAP1[0] PAD1                    N/A
  P1[19]/MC0A/USB_PPWR/N_CAP1.1     PAD2                    N/A
  P1[20]/MCFB0/PWM1.2/SCK0          PAD3                    N/A
  P1[21]/MCABORT/PWM1.3/SSEL0       PAD4                    N/A
  P1[22]/MC0B/USB-PWRD/MAT1.0       PAD5                    N/A
  P1[23]/MCFB1/PWM1.4/MISO0         PAD6                    N/A
  P1[24]/MCFB2/PWM1.5/MOSI0         PAD7                    N/A
  P1[25]/MC1A/MAT1.1                PAD8                    N/A
  P1[26]/MC1B/PWM1.6/CAP0.0         PAD9                    N/A
  P1[27]/CLKOUT/USB-OVRCR-N/CAP0.1  PAD10                   N/A
  P1[28]/MC2A/PCAP1.0/MAT0.0        PAD11                   N/A
  P1[29]/MC2B/PCAP1.1/MAT0.1        PAD12                   N/A
  P1[30]/VBUS/AD0[4]                J6-19                   AD0.4
  P1[31]/SCK1/AD0[5]                J6-20                   AD0.5

  P2[0]/PWM1.1/TXD1                 J6-42                   PWM1.1
  P2[1]/PWM1.2/RXD1                 J6-43                   PWM1.2
  P2[2]/PWM1.3/CTS1/TRACEDATA[3]    J6-44                   PWM1.3
  P2[3]/PWM1.4/DCD1/TRACEDATA[2]    J6-45                   PWM1.4
  P2[4]/PWM1.5/DSR1/TRACEDATA[1]    J6-46                   PWM1.5
  P2[5]/PWM1[6]/DTR1/TRACEDATA[0]   J6-47                   PWM1.6
  P2[6]/PCAP1[0]/RI1/TRACECLK       J6-48    
  P2[7]/RD2/RTS1                    J6-49    
  P2[8]/TD2/TXD2                    J6-50    
  P2[9]/USB_CONNECT/RXD2            PAD19   USB Pullup      N/A
  P2[10]/EINT0/NMI                  J6-51    
  P2[11]/EINT1/I2STX_CLK            J6-52    
  P2[12]/EINT2/I2STX_WS             j6-53    
  P2[13]/EINT3/I2STX_SDA            J6-27                 

  P3[25]/MAT0.0/PWM1.2              PAD13                   N/A
  P3[26]/STCLK/MAT0.1/PWM1.3        PAD14                   N/A

  P4[28]/RX-MCLK/MAT2.0/TXD3        PAD15                   N/A
  P4[29]/TX-MCLK/MAT2.1/RXD3        PAD16                   N/A

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

Code Red IDE
^^^^^^^^^^^^

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

  Using Code Red GNU Tools from Cygwin
  ------------------------------------

  Under Cygwin, the Code Red command line tools (e.g., arm-non-eabi-gcc) cannot
  be executed because the they only have execut privileges for Administrators.  I
  worked around this by:
  
  Opening a native Cygwin RXVT as Administrator (Right click, "Run as administrator"),
  then executing 'chmod 755 *.exe' in the following directories:

  /cygdrive/c/nxp/lpcxpreeso_3.6/bin, and
  /cygdrive/c/nxp/lpcxpreeso_3.6/Tools/bin

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
     ./configure.sh lpcxpresso-lpc1768/<sub-dir>

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

LEDs
^^^^

  If CONFIG_ARCH_LEDS is defined, then support for the LPCXpresso LEDs will be
  included in the build.  See:

  - configs/lpcxpresso-lpc1768/include/board.h - Defines LED constants, types and
    prototypes the LED interface functions.

  - configs/lpcxpresso-lpc1768/src/lpcxpresso_internal.h - GPIO settings for the LEDs.

  - configs/lpcxpresso-lpc1768/src/up_leds.c - LED control logic.

  The LPCXpresso has 3 LEDs... two on the Babel CAN board and a "heartbeat" LED."
  The LEDs on the Babel CAN board are capabl of OFF/GREEN/RED/AMBER status.
  In normal usage, the two LEDs on the Babel CAN board would show CAN status, but if
  CONFIG_ARCH_LEDS is defined, these LEDs will be controlled as follows for NuttX
  debug functionality (where NC means "No Change").

  During the boot phases.  LED1 and LED2 will show boot status.

                                          /* LED1   LED2   HEARTBEAT */
    #define LED_STARTED                0  /* OFF    OFF    OFF */
    #define LED_HEAPALLOCATE           1  /* GREEN  OFF    OFF */
    #define LED_IRQSENABLED            2  /* OFF    GREEN  OFF */
    #define LED_STACKCREATED           3  /* OFF    OFF    OFF */

    #define LED_INIRQ                  4  /*  NC     NC    ON  (momentary) */
    #define LED_SIGNAL                 5  /*  NC     NC    ON  (momentary) */
    #define LED_ASSERTION              6  /*  NC     NC    ON  (momentary) */
    #define LED_PANIC                  7  /*  NC     NC    ON  (0.5Hz flashing) */
    #undef  LED_IDLE                      /* Sleep mode indication not supported */

  After the system is booted, this logic will no longer use LEDs 1 and 2.  They
  are then available for use the application software using lpc17_led1() and
  lpc17_led2():

    enum lpc17_ledstate_e
    {
      LPC17_LEDSTATE_OFF   = 0,
      LPC17_LEDSTATE_GREEN = 1,
      LPC17_LEDSTATE_RED   = 2,
      LPC17_LEDSTATE_AMBER = (LPC17_LEDSTATE_GREEN|LPC17_LEDSTATE_RED),
    };

    EXTERN void lpc17_led1(enum lpc17_ledstate_e state);
    EXTERN void lpc17_led2(enum lpc17_ledstate_e state);

  The heartbeat LED is illuminated during all interrupt and signal procressing.
  Normally, it will glow dimly to inicate that the LPC17xx is taking interrupts.
  On an assertion PANIC, it will flash at 1Hz.

LPCXpresso Configuration Options
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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

	   CONFIG_ARCH_BOARD=lpcxpresso-lpc1768

	CONFIG_ARCH_BOARD_name - For use in C code

	   CONFIG_ARCH_BOARD_LPCEXPRESSO=y

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

  LPC17xx specific PHY/Ethernet device driver settings.  These setting
  also require CONFIG_NET and CONFIG_LPC17_ETHERNET.

	CONFIG_PHY_KS8721 - Selects Micrel KS8721 PHY
	CONFIG_PHY_AUTONEG - Enable auto-negotion
	CONFIG_PHY_SPEED100 - Select 100Mbit vs. 10Mbit speed.
	CONFIG_PHY_FDUPLEX - Select full (vs. half) duplex

    CONFIG_NET_EMACRAM_SIZE - Size of EMAC RAM.  Default: 16Kb
	CONFIG_NET_NTXDESC - Configured number of Tx descriptors. Default: 18
	CONFIG_NET_NRXDESC - Configured number of Rx descriptors. Default: 18
	CONFIG_NET_PRIORITY - Ethernet interrupt priority.  The is default is
	  the higest priority.
	CONFIG_NET_WOL - Enable Wake-up on Lan (not fully implemented).
	CONFIG_NET_REGDEBUG - Enabled low level register debug.  Also needs
	  CONFIG_DEBUG.
	CONFIG_NET_DUMPPACKET - Dump all received and transmitted packets.
	  Also needs CONFIG_DEBUG.
	CONFIG_NET_HASH - Enable receipt of near-perfect match frames.
	CONFIG_NET_MULTICAST - Enable receipt of multicast (and unicast) frames.
      Automatically set if CONFIG_NET_IGMP is selected.

  LPC17xx USB Device Configuration

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

  LPC17xx USB Host Configuration (the LPCXpresso does not support USB Host)

    CONFIG_USBHOST_OHCIRAM_SIZE
      Total size of OHCI RAM (in AHB SRAM Bank 1)
    CONFIG_USBHOST_NEDS
      Number of endpoint descriptors
    CONFIG_USBHOST_NTDS
      Number of transfer descriptors
    CONFIG_USBHOST_TDBUFFERS
      Number of transfer descriptor buffers
    CONFIG_USBHOST_TDBUFSIZE
      Size of one transfer descriptor buffer
    CONFIG_USBHOST_IOBUFSIZE
      Size of one end-user I/O buffer.  This can be zero if the
      application can guarantee that all end-user I/O buffers
      reside in AHB SRAM.

Configurations
^^^^^^^^^^^^^^

Each LPCXpresso configuration is maintained in a sudirectory and can be
selected as follow:

	cd tools
	./configure.sh lpcxpresso-lpc1768/<subdir>
	cd -
	. ./setenv.sh

Where <subdir> is one of the following:

  ostest:
    This configuration directory, performs a simple OS test using
    examples/ostest.
