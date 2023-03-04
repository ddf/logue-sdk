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
CXXSRC = unit.cc

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

UDEFS = 

