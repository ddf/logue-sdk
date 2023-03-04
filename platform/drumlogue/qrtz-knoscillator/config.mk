##############################################################################
# Project Configuration
#

PROJECT := knoscillator
PROJECT_TYPE := synth

##############################################################################
# Sources
#

# C sources
CSRC = header.c

# C++ sources
CXXSRC = unit.cc matrix.cc

# List ASM source files here
ASMSRC = 

ASMXSRC = 

##############################################################################
# Include Paths
#

UINCDIR  = $(realpath $(PROJECT_ROOT)/../../ext/OwlProgram/Source/)
UINCDIR += $(realpath $(PROJECT_ROOT)/../../ext/OwlProgram/LibSource/)
UINCDIR += $(realpath $(PROJECT_ROOT)/../../ext/OwlPatches/OwlPatches/)

##############################################################################
# Library Paths
#

ULIBDIR = 

##############################################################################
# Libraries
#

ULIBS  = -lm
ULIBS += -lc

##############################################################################
# Macros
#

# Need this to disable the ASSERT macro in OwlProgram/LibSource/message.h
# because when it is active the unit fails to load on the device.
UDEFS = -DNDEBUG

