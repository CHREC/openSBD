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

#
# Include paths
INCLUDES+=-I$(OSPL_HOME)/include/dcps/C++/SACPP
INCLUDES+=-I$(OSPL_HOME)/include
INCLUDES+=-I$(OSPL_HOME)/include/sys

# C++ Compiler settings.
CXX=g++
CXXFLAGS=-g -c -fPIC -m32
CPPFLAGS=$(INCLUDES)

# Linker settings.
LD_SO=$(CXX)
LD_FLAGS=-shared -m32
LD_LIBS=-lstdc++

TARGET=libSBCommon.so

# OpenSplice idl preprocessor
OSPLICE_COMP=$(OSPL_HOME)/exec/idlpp -S -l cpp

OSPLICE_LIBS=-lddsdatabase -ldcpsgapi -ldcpssacpp -lddsos -lddskernel

DCPS_SRC_FILES = SB.cpp SBDcps_impl.cpp SBDcps.cpp SBSplDcps.cpp
DCPS_OBJ_FILES = $(patsubst %.cpp, %.o, $(DCPS_SRC_FILES))

all : $(TARGET)
	@echo ">>>> all done" 

$(TARGET) : $(DCPS_OBJ_FILES)
	$(LD_SO) $(LD_FLAGS) -L$(OSPL_HOME)/lib $(OSPLICE_LIBS) $(LD_LIBS) -o $(TARGET) $(DCPS_OBJ_FILES)

$(DCPS_SRC_FILES) : SB.idl
	@echo "Compiling $< with SPLICE IDL compiler"
	@$(OSPLICE_COMP) $(INCLUDES) $<

clean :  
	@rm -f *.cpp *.h
	@rm -f *.o ospl-info.log
	@rm -f *.so
	@rm -f bld/*
	@rmdir bld

