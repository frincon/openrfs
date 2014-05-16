README for OpenRFS
==================
[![Build Status](https://travis-ci.org/frincon/openrfs.png?branch=master)](https://travis-ci.org/frincon/openrfs)

OpenRFS
-------
OpenRFS aims to be a replacement for replicated file system. With OpenRFS you may have sincronized filesystems 
between two machines in real time.

OpenRFS not implement any filesystem at low level, it simply envolve a directory with FUSE and log all operations
in a database for send to another machine. It uses librsync to send changes, that makes no need for send entire
large files when changes are small.


Quick Installation
------------------

Linux: Make sure you have basic development tools and the kernel includes 
the FUSE kernel module. Then unpack the source tarball and type:

    ./configure
    make
    make install # or 'sudo make install' if you aren't root.
  

