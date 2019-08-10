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

set -e -x

#### #### #### 
# Git & Clone 
#### #### #### 

# Download open source cFE and OpenSplice (these are the latest available releases currently)
# git clone -b '6.5.0a' https://github.com/nasa/cfe.git

#### git clone --recurse-submodules https://github.com/nasa/icarous.git

#### cd icarous

#### git clone -b DDS https://github.com/CHREC/openSBD DDS

JARVIS_HOME=$1
DDS_HOME=$PWD/DDS


if [[ "$(basename $2)" == "OSPL" ]]
	then 
		cd DDS
		echo -e "\n Cloning Repos...\n"
 		git clone -b 'OSPL_V6_9_190403OSS_RELEASE' --depth 1 https://github.com/ADLINK-IST/opensplice.git

		echo -e "\n ... Building OSPL...\n"

		#### #### #### #####
		# Build OpenSplice #
		#### #### #### ##### 

		cd $DDS_HOME/opensplice
		export INCLUDE_SERVICES_CMSOAP=no
		echo '15' | ./configure 
		# [15:x86_64.linux-release] [21: x86] ## ~source
		make
		make install
fi

echo -e "\n ... Sourcing OSPL vars...\n"

export OSPL_HOME="$DDS_HOME/opensplice/install/HDE/x86_64.linux"
echo -e "OSPL_HOME=${OSPL_HOME}\n\n"
source $OSPL_HOME/release.com	# opensplice/install/HDE/x86_64.linux/release.com

#### #### #### #### #### #### 
# Configure and Set ICAROUS
#### #### #### #### #### #### 

cd $JARVIS_HOME

echo -e "\n ... Setting Envrionment [Check Your Paths!]...\n"

source SetEnv.sh

echo -e "\n JAVA_HOME=${JAVA_HOME}"
echo -e "OSAL_HOME=${OSAL_HOME}"
echo -e "PLEXIL_HOME=${PLEXIL_HOME}"
echo -e "OSPL_HOME=${OSPL_HOME}"

echo -e "\n\n\n"

cd  $JARVIS_HOME
rm -rf build
mkdir build

#### #### #### #### 
# Patch and Build
#### #### #### #### 

cp $DDS_HOME/patches/opensplice64.patch $DDS_HOME/opensplice
cd $DDS_HOME/opensplice

patch -Np1 -i $DDS_HOME/patches/opensplice64.patch

echo -e "\n ... Building SBD Components...\n"

cd $DDS_HOME/code
make

cp *.{c,cpp,h} $JARVIS_HOME/cFS/cfe/fsw/cfe-core/src/sb
cp $DDS_HOME/code/libSBCommon.so $OSPL_HOME/lib
cp $DDS_HOME/code/rtps.ini $JARVIS_HOME/cFS/bin/cpu1 

cd $JARVIS_HOME/cFS/cfe

echo -e "\n ... Patching cFS ...\n"

cp $DDS_HOME/patches/sbd64.patch $JARVIS_HOME/cFS/
cd $JARVIS_HOME/cFS/

patch -Np1 -i $DDS_HOME/patches/sbd64.patch

echo -e "\n ... Reticulating cFS CMake ...\n"

cd $JARVIS_HOME/cFS/cfe/

patch -Np1 -i $DDS_HOME/patches/cfe__cmake__target__CMakeLists.txt.patch

patch -Np1 -i $DDS_HOME/patches/cfe__fsw__cfe-core__CMakeLists.txt.patch

echo -e "\n ... Entering JARVIS build ...\n"

cd $JARVIS_HOME/build
cp $DDS_HOME/code/SB.idl $JARVIS_HOME/cFS/cfe/fsw/cfe-core/src/sb

cmake -DCMAKE_MODULE_PATH=$JARVIS_HOME/CMake ..
make cpu1-install -j9

cd $JARVIS_HOME/cFS/bin/cpu1/cf
rm cfe_es_startup.scr # cpu1_cfe_es_startup.scr
ln -s $DDS_HOME/scenarios/JARVIS.scr cfe_es_startup.scr

