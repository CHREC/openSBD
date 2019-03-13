# SBD - SHREC Redistribution

Copyright (c) 2018 NSF Center for Space, High-performance, and Resilient Computing (SHREC)
University of Pittsburgh. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided
that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.

# SBD - Network Framework for cFS

SBD (Software Bus - Distributed) is a modified version of NASA-GSFC cFS's
cFE Software Bus (SB) core service that uses DDS to pass messages between 
applications, without cFS application redesign.

The scripts mentioned in the build and run sections of this document are
meant to provide an example of how to incorporate SBD into a cFE source
tree, then build and run cFS using cFE with the SB-DDS mod.

This software was initially developed with OpenDDS implementation (defunct); in
order to allow greater compatability with DDSI-RTPS (DDSv2) and p2p peer-discovery, 
the SBD was reimplemented via OpenSplice (OSPL). OSPL enabled greater features 
and functionality, with less overhead through C/C++ stand alone implementation where
CORBA technologies are not required. 

This version of OSPL is the 6.x Community Edition (CE). In 6.x, DLRL is replaced 
by functions of the IDL Pre-Processor methods. DDS may be used in a shared-memory 
mode, rather than being launched as a sepearte service (though daemon mode is still
available, and better fitted in some scenarios). This release of SBD leverages 
OSPL via shared mode rather than running the daemon.

## Dependencies

cFE version: 6.5.0a
OpenSplice version: 6.x

## Limitations

### Software
 - "Zerocopy" support is disabled.

### Pitfalls
 - Locating `rtps.ini` and `ospl.xml` with `file:///<path-after-root/to/file>` in $OSPL_URI
 - Setting `LD_LIBRARY_PATH` for finding all libraries during build process.
 - Adjust -lwrap: in /lib/i386-linux-gnu, set a symlink: `ln -s libwrap.so.0 libwrap.so`
 - Managing topics for publish-subscribe models in distributed systems; when configuring cFS applications: SB_MSG ID's should not conflict with each other across nodes- N.B., applications on different nodes, sharing the same topic IDs, will see each others' traffic.

## ARM Build

To build on ARM platform: make sure to run an ARM compliant cFS Make

## Running cFE 

Make sure to use the soap-syntax "file:///" for directory reference for the {ospl}.xml config directory for OSPL_URI, which is required in run-time for OSPL_DDS to allow the SB to initialize.

## Masked MSG ID's

 - To use the DDS network, applications should a specific range of messages.
 - The preset fitlers for cFS SB messages are:
   1. Minimum Topic ID    	`0x18C0`
   2. Maximum Topic ID 		`0x18DF`









