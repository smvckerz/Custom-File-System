# File: Standard Makefile for CSC415
#
# Description - This make file should be used for all your projects
# It should be modified as needed for each homework
#
# ROOTNAME should be set you your lastname_firstname_HW.  Except for
# and group projects, this will not change throughout the semester
#
# HW should be set to the assignment number (i.e. 1, 2, 3, etc.)
#
# FOPTION can be set to blank (nothing) or to any thing starting with an 
# underscore (_).  This is the suffix of your file name.
#
# With these three options above set your filename for your homework
# assignment will look like:  bierman_robert_HW1_main.c 
#
# RUNOPTIONS can be set to default values you want passed into the program
# this can also be overridden on the command line
#
# OBJ - You can append to this line for additional files necessary for
# your program, but only when you have multiple files.  Follow the convention
# but hard code the suffix as needed.
#
# To Use the Makefile - Edit as above
# then from the command line run:  make
# That command will build your program, and the program will be named the same
# as your main c file without an extension.
#
# You can then execute from the command line: make run
# This will actually run your program
#
# Using the command: make clean
# will delete the executable and any object files in your directory.
#

FIRSTNAME=Eduardo
LASTNAME=Munoz

ROOTNAME=$(LASTNAME)_$(FIRSTNAME)_HW
HW=4
FOPTION=_main

# RUNOPTIONS
RUNOPTIONS=Law500K.dat header.txt 1 police_district BAYVIEW MISSION

CC=gcc
CFLAGS= -g -I.
# To add libraries, add "-l <libname>", for multiple repeat prior for each lib.
LIBS =-l pthread -lm
DEPS = 
ARCH = $(shell uname -m)

# Add additional files here to the ADDOBJ, add the .o extension of the file only
#     i.e. something along the line of Bierman_Robert_HW4_datastruct.o
ADDOBJ =

OBJ = $(ROOTNAME)$(HW)$(FOPTION).o $(ADDOBJ)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ROOTNAME)$(HW)$(FOPTION): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)  $(LIBS)

clean:
	rm $(ROOTNAME)$(HW)$(FOPTION).o $(ROOTNAME)$(HW)$(FOPTION) $(ADDOBJ)

run: $(ROOTNAME)$(HW)$(FOPTION)
	./$(ROOTNAME)$(HW)$(FOPTION) $(RUNOPTIONS)

vrun: $(ROOTNAME)$(HW)$(FOPTION)
	valgrind ./$(ROOTNAME)$(HW)$(FOPTION) $(RUNOPTIONS)


