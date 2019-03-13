#! /usr/bin/env bash

# Copyright (c) 2018 NSF Center for Space, High-performance, and Resilient Computing (SHREC)
# University of Pittsburgh. All rights reserved.

# Redistribution and use in source and binary forms, with or without modification, are permitted provided
# that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
# OF SUCH DAMAGE.


echo -e "\n-- Source OpenSplice Libraries into Shell...\n"

# SET OSAL_HOME
# SET OSPL_HOME
# SET JAVA_HOME
 
# ADD LIBRARIES TO PATH
# ADD LIBRARIES TO LD_LIBRARY_PATH
 
# SET OSPL_URI

echo -e "\n-- Generate SB DDS Source Files...\n"

# MAKE dcps/sb DIR
# COPY IDL to 'sb' for build
# COPY Makefile to 'sb' for build 

# GOTO OSPL 'sb' directory
# MAKE OSPL 'sb'

echo -e "\n-- Finished generating SB DDS Source Files...\n"

echo -e "\n-- Building cFE. Thank you NASA!...\n"

# COPY code INTO cfe sb src
# COPY OSPL generated 'sb' into cfe sb src
# COPY rtps.ini into cFS exe build

#setvars for cFS 

echo -e "\n-- Environment variables set up...\n"

# PATCH cFS
# CHECK LIBRARIES 
# MAKE cFS

echo -e "\nSBD BUILD COMPLETE!\n"

# SOURCE OSPL release.com
# SOURCE OSPL_URI
# PRELOAD libSBCommon "if necessary"
# RUN cFS

