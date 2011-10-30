Application Folder
==================

Contents
--------

  General
  Directory Location
  Named Applications
  Named Startup main() function
  NuttShell (NSH) Built-In Commands
  Application Configuration File
  Example Named Application

General
-------
This folder provides various applications found in sub-directories.  These
applications are not inherently a part of NuttX but are provided you help
you develop your own applications.  The apps/ directory is a "break away"
part of the configuration that you may chose to use or not.

Directory Location
------------------
The default application directory used by the NuttX build should be named
apps/ (or apps-x.y/ where x.y is the NuttX version number).  This apps/
directoy should appear in the directory tree at the same level as the
NuttX directory.  Like:

 .
 |- nuttx
 |
 `- apps

If all of the above conditions are TRUE, then NuttX will be able to
find the application directory.  If your application directory has a 
different name or is location at a different position, then you will
have to inform the NuttX build system of that location.  There are several
ways to do that:

1) You can define CONFIG_APPS_DIR to be the full path to your application
   directory in the NuttX configuration file.
2) You can provide the path to the application directory on the command line
   like:  make APPDIR=<path> or make CONFIG_APPS_DIR=<path>
3) When you configure NuttX using tools/configure.sh, you can provide that
   path to the application directory on the configuration command line
   like: ./configure.sh -a <app-dir> <board-name>/<config-name>

Named Applications
------------------
NuttX also supports applications that can be started using a name string.
In this case, zpplication entry points with their requirements are gathered
together in two files:

  - namedapp/namedapp_proto.h  Entry points, prototype function
  - namedapp/namedapp_list.h  Application specific information and requirements

The build occurs in several phases as different build targets are executed:
(1) context, (2) depend, and (3) default (all). Application information is
collected during the make context build phase.

To execute an application function:

  exec_namedapp() is defined in the nuttx/include/apps/apps.h 

NuttShell (NSH) Built-In Commands
---------------------------------
One use of named applications is to provide a way of invoking your custom
application through the NuttShell (NSH) command line.  NSH will support
a seamless method invoking the applications, when the following option is
enabled in the NuttX configuration file:

  CONFIG_NSH_BUILTIN_APPS=y

Application Configuration File
------------------------------
A special configuration file is used to configure which applications
are to be included in the build.  The source for this file is 
configs/<board>/<configuration>/appconfig.  The existence of the appconfig
file in the board configuration directory is sufficient to enable building
of applications.

The appconfig file is copied into the apps/ directory as .config when
NuttX is configured.  .config is included in the toplevel apps/Makefile.
As a minimum, this configuration file must define files to add to the
CONFIGURED_APPS list like:

  CONFIGURED_APPS  += vsn/hello vsn/poweroff vsn/jvm

Named Start-Up main() function
------------------------------
A named application can even be used as the main, start-up entry point
into your embedded software.  When the user defines this option in
the NuttX configuration file:

  CONFIG_BUILTIN_APP_START=<application name>
  
that application shall be invoked immediately after system starts
*instead* of the normal, default "user_start" entry point.
Note that <application name> must be provided as: "hello", 
will call:

  int hello_main(int argc, char *argv[])

Example Named Application
-------------------------
An example application skeleton can be found under the vsn/hello
sub-directory.  This example shows how a named application can be added
to the project. One must define:

 1. create sub-directory as: appname
 2. provide entry point: appname_main()
 3. set the requirements in the file: Makefile, specially the lines:

  APPNAME    = appname
  PRIORITY  = SCHED_PRIORITY_DEFAULT
  STACKSIZE  = 768
  ASRCS    = asm source file list as a.asm b.asm ...
  CSRCS    = C source file list as foo1.c foo2.c ..

 4. add application in the apps/.config




