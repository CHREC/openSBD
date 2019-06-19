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

# This script should be all that is needed to go from a new clone of openSBD to
# a running barebones cFE with integrated OpenSplice SBD, assuming all component
# prerequisites are installed. Tested on Ubuntu 16.04 64-bit.
#
# Note 1:
# It is recommended that the beginning section (OpenSplice compile) is done
# manually in case of build errors.
#
# Note 2:
# Enter target 21 (for x86.linux-release) when prompted during OpenSplice configure
#### #### #### #### #### #### #### #### #### #### #### #### #### #### #### #### #### 

SBD_HOME=$PWD

# Download open source cFE and OpenSplice (these are the latest available releases currently)
# git clone -b '6.5.0a' https://github.com/nasa/cfe.git

#### #### #### 
# Git & Clone 
#### #### #### 

echo -e "\n Cloning Repos...\n"

#### git clone https://github.com/CHREC/openSBD
#### cd openSBD
git clone --recurse-submodules https://github.com/nasa/icarous.git
git clone -b 'OSPL_V6_9_190403OSS_RELEASE' --depth 1 https://github.com/ADLINK-IST/opensplice.git
#### git clone -b '1d0b2e660dfc332d786eef44bf8f608c9319af57' https://gitlab.larc.nasa.gov/larc-nia-fm/icarous

#### #### #### #### #### #### 
# Configure and Set ICAROUS
#### #### #### #### #### #### 

cd icarous

echo -e "\n ... Setting Envrionment [Check Your Paths!]...\n"
# JAVA_HOME=/usr/lib/jvm/java-1.8.0-openjdk-1.8.0.212.b04-0.el7_6.x86_64/include

source SetEnv.sh

echo -e "\n JAVA_HOME=${JAVA_HOME}"
echo -e "OSAL_HOME=${OSAL_HOME}"
echo -e "PLEXIL_HOME=${PLEXIL_HOME}"

mkdir build


echo -e "\n ... Building OSPL...\n"

#### #### #### #### 
# Build OpenSplice
#### #### #### #### 

cd $SBD_HOME/opensplice
export INCLUDE_SERVICES_CMSOAP=no
source ./configure # Enter target 21 (x86.linux-release)
make
make install

echo -e "\n ... Sourcing OSPL vars...\n"

export OSPL_HOME="$SBD_HOME/opensplice/install/HDE/x86.linux"
echo -e "OSPL_HOME=${OSPL_HOME}\n\n"
source $OSPL_HOME/release.com

#### #### #### #### 
# Patch and Build
#### #### #### #### 

cp $SBD_HOME/patches/sbd.patch opensplice
cd $SBD_HOME/opensplice

patch -Np1 -i $SBD_HOME/patches/opensplice.patch

echo -e "\n ... Building SBD Components...\n"

cd $SBD_HOME/code
make
cp *.{c,cpp,h} $SBD_HOME/icarous/cFS/cFE/cfe/fsw/cfe-core/src/sb
cp $SBD_HOME/code/libSBCommon.so $OSPL_HOME/lib
cp $SBD_HOME/code/rtps.ini $SBD_HOME/icarous/build/cpu1/exe

cd $SBD_HOME/icarous/cFS/cFE
# git submodule update --init osal

echo -e "\n ... Patching cFS ...\n"

cp $SBD_HOME/patches/sbd.patch $SBD_HOME/icarous/cFS/cFE/
cd $SBD_HOME/icarous/cFS/cFE/


patch -Np1 -i $SBD_HOME/patches/sbd.patch

# source setvars.sh
#### cd $SBD_HOME/icarous/build

cp $SBD_HOME/patches/sbd.patch $SBD_HOME/icarous/cFS/cFE/
cd $SBD_HOME/icarous/cFS/cFE/ 

patch -Np1 -i $SBD_HOME/patches/cfe__cmake__target__CMakeLists.txt.patch
patch -Np1 -i $SBD_HOME/patches/cfe__fsw__cfe-core__CMakeLists.txt.patch

echo -e "\n ... Entering ICAROUS build ...\n"

cd $SBD_HOME/icarous/build
cp $SBD_HOME/code/SB.idl $SBD_HOME/icarous/cFS/cFE/cfe/fsw/cfe-core/src/sb

# cmake CMAKE_PREFIX_PATH=$OSPL_HOME ..

cmake -DCMAKE_MODULE_PATH=$SBD_HOME/icarous/CMake ..
make cpu1-install #-j9

#### cp $SBD_HOME/code/rtps.ini $SBD_HOME/icarous/cFS/bin
# sudo sysctl fs.mqueue.msg_max=1000 #
#### cd $SBD_HOME/icarous/bin
#### ./core-cpu1 --scid=88

#### #### 
# Run
#### #### 

#cd exe
#cp $SBD_HOME/code/rtps.ini $SBD_HOME/cfe/build/cpu1/exe
#./core-linux.bin
