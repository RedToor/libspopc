
 VMS (OpenVMS):
   
   On VMS a command procedure is provided to build the kit.
   It must be run from the main directory.

 - To build with SSL support use

    $ set default <kit-main-directory> ! the directory where is AUTHORS.;
    $ @[.vms]build_libspopc.com SSL

   It requires HP SSL.
   It creates the object library LIBSPOPC_SSL.OLB

   To link with your modules:

    $ LINK your_modules,LIBSPOPC_DIR:LIBSPOPC_SSL.OLB/LIBRARY, -
            SYS$INPUT/opt
       SYS$LIBRARY:SSL$LIBSSL_SHR32/share
       SYS$LIBRARY:SSL$LIBCRYPTO_SHR32/share


 - To build without SSL support

    $ set default <kit-main-directory> ! the directory where is AUTHORS.;
    $ @[.vms]build_libspopc.com

   It creates the object library LIBSPOPC.OLB

   To link with your modules:

    $ LINK your_modules,LIBSPOPC_DIR:LIBSPOPC.OLB/LIBRARY

 - To run a test program (subdirectory [.examples]) you must define it 
   as foreign command.
   
 - On OpenVMS the reentrancy code is not yet supported.

 2009 - Francesco Gennai - francesco.gennai@isti.cnr.it
