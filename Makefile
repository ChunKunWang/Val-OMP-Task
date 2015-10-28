# Example Makefile for ROSE users
# This makefile is provided as an example of how to use ROSE when ROSE is
# installed (using "make install").  This makefile is tested as part of the
# "make distcheck" rule (run as part of tests before any SVN checkin).
# The test of this makefile can also be run by using the "make installcheck"
# rule (run as part of "make distcheck").
#
# Chun-Kun Wang (amos@cs.unc.edu)

# Location of include directory after "make install"
ROSE_INCLUDE_DIR = /home/G2009/amos/test/compileTree/include

# Location of Boost include directory
BOOST_CPPFLAGS = -pthread -I/home/G2009/amos/test/installTree/include

# Location of Dwarf include and lib (if ROSE is configured to use Dwarf)
ROSE_DWARF_INCLUDES = 
ROSE_DWARF_LIBS_WITH_PATH = 

# Location of library directory after "make install"
ROSE_LIB_DIR = /home/G2009/amos/test/compileTree/lib

CC                    = gcc
CXX                   = g++
CPPFLAGS              =  -I/usr/lib/jvm/java-6-sun/include -I/usr/lib/jvm/java-6-sun/include/linux
#CXXCPPFLAGS           = @CXXCPPFLAGS@
CXXFLAGS              = 
LDFLAGS               = 

ROSE_LIBS = $(ROSE_LIB_DIR)/librose.la

# Location of source code
ROSE_SOURCE_DIR = .

executableFiles = amos\

#AmosValidation
OBJ = amos.o amosValidation.o
OBJDIR = obj

# Default make rule to use
all: $(executableFiles)
	@if [ x$${ROSE_IN_BUILD_TREE:+present} = xpresent ]; then echo "ROSE_IN_BUILD_TREE should not be set" >&2; exit 1; fi

# Example of how to use ROSE (linking to dynamic library, which is must faster
# and smaller than linking to static libraries).  Dynamic linking requires the 
# use of the "-L$(ROSE_LIB_DIR) -Wl,-rpath" syntax if the LD_LIBRARY_PATH is not
# modified to use ROSE_LIB_DIR.  We provide two example of this; one using only
# the "-lrose -ledg" libraries, and one using the many separate ROSE libraries.
$(executableFiles): ${OBJDIR} $(OBJ)
	@/bin/sh /home/G2009/amos/test/compileTree/libtool --mode=link $(CXX) $(CPPFLAGS) $(CXXFLAGS)  $(LDFLAGS) -I$(ROSE_INCLUDE_DIR) $(BOOST_CPPFLAGS) $(ROSE_DWARF_INCLUDES) -o $@ amos.o amosValidation.o $(ROSE_LIBS) $(ROSE_DWARF_LIBS_WITH_PATH)
	@mv $(OBJ) $(OBJDIR)
	@echo
	@echo "$(executableFiles)make successfully!"
	@echo

amos.o:amos.cpp amosValidation.h
	@/bin/sh /home/G2009/amos/test/compileTree/libtool --mode=install $(CXX) $(CPPFLAGS) $(CXXFLAGS)  $(LDFLAGS) -I$(ROSE_INCLUDE_DIR) $(BOOST_CPPFLAGS) $(ROSE_DWARF_INCLUDES) -c amos.cpp $(ROSE_LIBS) $(ROSE_DWARF_LIBS_WITH_PATH)

amosValidation.o:amosValidation.cpp amosValidation.h
	@/bin/sh /home/G2009/amos/test/compileTree/libtool --mode=install $(CXX) $(CPPFLAGS) $(CXXFLAGS)  $(LDFLAGS) -I$(ROSE_INCLUDE_DIR) $(BOOST_CPPFLAGS) $(ROSE_DWARF_INCLUDES) -c amosValidation.cpp $(ROSE_LIBS) $(ROSE_DWARF_LIBS_WITH_PATH)

$(OBJDIR):
	@mkdir $(OBJDIR)

clean:
	@rm -rf $(executableFiles) a.out *.dot *.o *.ti temp_*.c temp_*.cpp rose_*.c rose_*.cpp amos_*.c amos_*.cpp amos_*.C amos_*.CPP
	@rm -f $(OBJDIR)/*.o
	@rm -rf amos_tmp
	@echo "Clean complete!"

