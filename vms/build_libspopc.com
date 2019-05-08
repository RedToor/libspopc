$! BUILD_LIBSPOPC.COM
$!
$! Build the LIBSPOPC library
$!
$!  P1 is a comma-seperated list of parameters
$!      SSL             build with SSL support (requires HP SSL)
$!      DEBUG           enables debugging (implies LIST)
$!      LIST            generates listings
$!      FULL            Print all listing information
$!
$! This procedure creates an object library LIBSPOPC_DIR:LIBSPOPC.OLB.
$! After the library is built, you can link LIBSPOPC routines into
$! your code with the command
$!
$! With SSL support
$!      $ LINK your_modules,LIBSPOPC_DIR:LIBSPOPC_SSL.OLB/LIBRARY, -
$!               SYS$INPUT/opt
$!                 SYS$LIBRARY:SSL$LIBSSL_SHR32/share
$!                 SYS$LIBRARY:SSL$LIBCRYPTO_SHR32/share
$!
$! Without SSL support
$!      $ LINK your_modules,LIBSPOPC_DIR:LIBSPOPC.OLB/LIBRARY
$! 
$! Change History
$! --------------
$! Command file author : Francesco Gennai (francesco.gennai@isti.cnr.it)
$! Update history      : 07 January 2009 Francesco Gennai (francesco.gennai@isti.cnr.it)
$!                     : First release 
$! 
$!  Build options
$!
$   arch_name = f$getsyi("ARCH_NAME")
$!
$   inP1 = ",''P1',"
$   if f$locate(",DEBUG,", inP1) .NE. f$length(inP1)
$    then
$     debug_flag = "Y"
$     cc_command = "CC/DEBUG/NOOPTIMIZE"
$    else
$     debug_flag = "N"
$     cc_command = "CC"
$   endif
$   if f$locate(",LIST,", inP1) .NE. f$length(inP1) .OR. debug_flag
$    then
$     cc_command = cc_command + "/LIST"
$   endif
$   if f$locate(",FULL,", inP1) .NE. f$length(inP1)
$    then
$     cc_command = cc_command + "/SHOW=ALL"
$   endif
$   if f$locate(",SSL,", inP1) .NE. f$length(inP1)
$    then
$     if f$search("SYS$LIBRARY:SSL$LIBSSL_SHR32.EXE") .eqs. ""
$      then
$       write sys$output ""
$       write sys$output "Library SYS$LIBRARY:SSL$LIBSSL_SHR32.EXE not found"
$       write sys$output "You should install HP SSL or build without SSL support"
$       write sys$output ""
$       exit
$     endif
$     if f$search("SYS$LIBRARY:SSL$LIBCRYPTO_SHR32.EXE") .eqs. ""
$      then
$       write sys$output ""
$       write sys$output "Library SYS$LIBRARY:SSL$LIBCRYPTO_SHR32.EXE not found"
$       write sys$output "You should install HP SSL or build without SSL support"
$       write sys$output ""
$       exit
$     endif
$     ssl_flag = "Y"
$     cc_command = cc_command + "/define=use_ssl"
$    else
$     ssl_flag = "N"
$   endif
$!
$!  list of sources to be built into the LIBSPOPC library.
$!
$   sources = "format.c libspopc.c objects.c parsing.c queries.c session.c"
$!
$!  set up build logicals 
$!
$!   whoami = f$environment("PROCEDURE")
$   saved_default = f$environment("default")
$!   procdir = f$parse(whoami,,,"DIRECTORY") - ">[" - "><" - "]<" - "][" - "<" - "[" - ">" - "]"
$!   procdir = "[" + procdir + "]"
$!   procdev = f$parse(whoami,,,"DEVICE")
$!   procdevdir = procdev + procdir
$   procdevdir = saved_default
$!   set default 'procdevdir'
$!
$   if f$trnlnm("LIBSPOPC_DIR") .eqs. ""
$    then
$     elog = 0
$     write sys$output " "
$     write sys$output "Defining LIBSPOPC_DIR as ""''procdevdir'"""
$     define/process/nolog LIBSPOPC_DIR 'procdevdir'
$    else
$     elog = 1
$     write sys$output " "
$     write sys$output "Using existing LIBSPOPC_DIR: """+f$trnlnm("LIBSPOPC_DIR")+""""
$   endif
$!
$   libdir = "LIBSPOPC_DIR:"
$   exedir = libdir
$!
$   sslnm = ""
$   if ssl_flag 
$    then 
$     library_file = "LIBSPOPC_DIR:LIBSPOPC_SSL.OLB"
$    else
$     library_file = "LIBSPOPC_DIR:LIBSPOPC.OLB"
$   endif
$!
$   if f$search(library_file) .eqs. ""
$    then
$     write sys$output "Creating new object library ''library_file'"
$     library/create/object 'library_file'
$   endif
$!
$   libspopcopt = "libspopc.opt"
$   exit_status = 1
$!
$   on warning then goto ERROR_OUT
$   on control_y then goto CTRLY_OUT
$!
$   lib_command   = "LIBRARY/REPLACE/OBJECT ''library_file'"
$   link_command  = ""
$!
$   msg = "with NO SSL support"
$   if ssl_flag then msg = "WITH SSL SUPPORT"
$   write sys$output ""
$   write sys$output "Building modules ''msg' into the LIBSPOPC object library"
$   write sys$output ""
$!
$ index = 0
$ sources = f$edit(sources,"COMPRESS,TRIM")
$!
$ source_loop:
$!
$   next_source = f$edit(f$element( index," ",sources),"COLLAPSE,TRIM")
$   if f$length(next_source) .ne. 0
$    then
$!
$     on error then goto ERROR_OUT
$     on control_y then goto CTRLY_OUT
$     call build 'next_source'
$     index = index + 1
$     goto source_loop
$!
$   endif
$!
$!   now build example programs
$!
$!
$   if ssl_flag 
$    then 
$     open/write out 'libspopcopt'
$     write out "SYS$LIBRARY:SSL$LIBSSL_SHR32/share"
$     write out "SYS$LIBRARY:SSL$LIBCRYPTO_SHR32/share"
$     close out
$   endif
$!
$   set def [.examples]
$   exedir = f$environment("DEFAULT")
$   libdir = exedir
$!
$  q1:
$   t_flag ="Y"
$   write sys$output ""
$   read/end=exit_out -
        /prompt="Do you want build the example programs ''msg'? " -
         sys$command ans
$   if (f$edit(ans,"TRIM,UPCASE") .nes. "YES") .AND. -
         (f$edit(ans,"TRIM,UPCASE") .nes. "Y")
$    then
$     if f$extract(0,1,f$edit(ans,"TRIM,UPCASE")) .nes. "N" then goto q1
$     t_flag = "N"
$     goto skip_build_test
$   endif
$   write sys$output ""
$   write sys$output "Building example programs ''msg'"
$   write sys$output ""
$!
$   lib_command   = ""
$   link_command  = "LINK"
$!
$   call build butcher.c  [examples]
$   call build poptest1.c [examples]
$   call build poptest2.c [examples]
$   call build poptest3.c [examples]
$!
$  skip_build_test:
$   write sys$output ""
$   write sys$output ""
$   write sys$output "**********   Installation tasks   **********"
$   write sys$output ""
$!   if .NOT.elog
$!    then
$     write sys$output " You should define the following logical in your system startup procedure"
$     write sys$output ""
$     write sys$output "$ define/system/exec LIBSPOPC_DIR  ''procdevdir'"
$     write sys$output ""
$!   endif
$   if t_flag
$    then
$     write sys$output " Example of foreign command to run a test program:"
$     write sys$output ""
$     write sys$output "$ poptest1 :== $''exedir'poptest1.''arch_name'_exe"
$     write sys$output ""
$   endif
$   write sys$output "To link your programs ''msg'"
$   write sys$output ""
$   if ssl_flag
$    then
$     write sys$output "$ LINK your_modules,LIBSPOPC_DIR:LIBSPOPC_SSL.OLB/LIBRARY, -"
$     write sys$output "    SYS$INPUT/opt"
$     write sys$output " SYS$LIBRARY:SSL$LIBSSL_SHR32/share"
$     write sys$output " SYS$LIBRARY:SSL$LIBCRYPTO_SHR32/share"
$     write sys$output "$"
$    else
$     write sys$output "$ LINK your_modules,LIBSPOPC_DIR:LIBSPOPC.OLB/LIBRARY"
$   endif
$!
$ EXIT_OUT:
$!
$ write sys$output " "
$ write sys$output "Performing clean-up..."
$ set default 'saved_default'
$ if f$search("*.''arch_name'_obj;-1") .nes. "" then -
           purge/nolog *.'arch_name'_obj
$ if f$search("''exedir'*.''arch_name'_exe;-1") .nes. "" then -
           purge/nolog 'exedir'*.'arch_name'_exe
$ if f$search(libspopcopt) .nes. "" then delete/nolog 'libspopcopt';*
$ write sys$output "Exiting."
$ write sys$output " "
$ exit 'exit_status
$!
$ CTRLY_OUT:
$ exit_status = 44 ! SS$_ABORT
$ write sys$output "CTRL/Y or CTRL/C detected; aborting operations"
$ goto EXIT_OUT
$
$ ERROR_OUT:
$ exit_status = $status
$ if f$integer(exit_status) .eq. 44 then goto CTRLY_OUT
$ write sys$output "''f$message(exit_status)'"
$ goto EXIT_OUT
$!
$!***
$ build: SUBROUTINE
$! BUILD subroutine.  Compile then insert into library or link as required
$!
$   on warning then goto EXIT_BUILD
$   on control_y then goto EXIT_CTRLY
$   source_file = P1
$   name = f$parse(source_file,,,"NAME")
$   object_file = f$parse("''libdir'.''arch_name'_OBJ",name,,,"SYNTAX_ONLY")
$   image_file = f$parse("''exedir'.''arch_name'_EXE",name,,,"SYNTAX_ONLY")
$!
$!   compile
$!
$   write sys$output "$ ''cc_command -"
$   write sys$output "    /object=''object_file' - 
$   write sys$output "    ''source_file'"
$   cc_command /object='object_file 'source_file'
$   write sys$output " "
$!
$!   insert into library if command defined
$!
$   if lib_command .nes. ""  then lib_command 'object_file'
$!
$!   link module if command defined
$   if link_command .nes. ""
$    then
$       opts = ""
$       ssllib = ""
$       sslnm = ""
$       if debug_flag then opts = "/DEBUG"
$       if ssl_flag 
$        then 
$         ssllib = ", LIBSPOPC.opt/options"
$         sslnm = "_SSL"
$       endif
$       write sys$output "$ ''link_command' ''opts' -"
$       write sys$output "    ''object_file', -"
$       write sys$output "    LIBSPOPC_DIR:LIBSPOPC''sslnm'.OLB/library''ssllib' -"
$       write sys$output "    /execut=''image_file'"
$       link_command'opts' 'object_file', -
                LIBSPOPC_DIR:LIBSPOPC'sslnm'.OLB/library'ssllib' -
                /execut='image_file'
$       write sys$output " "
$   endif
$!
$ exit_build:
$   exit $status
$!
$ exit_ctrly:
$   exit 44  ! (SS$_ABORT)
$!
$ ENDSUBROUTINE ! build
