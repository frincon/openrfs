README for opendfs
==================
[![Build Status](https://travis-ci.org/frincon/opendfs.png?branch=master)](https://travis-ci.org/frincon/opendfs)

OpenDFS
-------
OpenDFS aims to be a replacement for another distributed file system. With OpenDFS you may have sincronized filesystems 
between two machines in real time.

OpenDFS not implement any filesystem in low level, it simply envolve a directory with FUSE and log all operations
in a database for send to another machine. It uses librsync to send changes, that makes no need for send entire
large files when changes are small.



