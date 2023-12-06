# Makfile for translit contributed by Dominik Wujastyk <D.Wujastyk@ucl.ac.uk>
# Modified to provide UNIX compilation and commented by
# Jan Labanowski <jkl@ccl.net>
# Read comments in the Makefile and modify it to suit your needs
# When modifying, remember that you need to start all dependencies lines
# (lines which have colon ":" in them) from the first column, and
# lines below them have to start with a TAB character.
#
# Before you can make translit, you need to modify path.h file.
#

# if you do not use Gnu C, modify the line below to be:
# CC=cc
CC=gcc

OBJS=translit.o reg_exp.o reg_sub.o

# For OS/2, change lines below to be:
# translit.exe: $(OBJS)
#	$(CC) -O -o translit.exe $(OBJS) # line has initial tab

translit: $(OBJS)
	$(CC) -O -o translit.exe $(OBJS) # line has initial tab

# Dependencies

translit.o : translit.c paths.h reg_exp.h
reg_exp.o : reg_exp.c paths.h reg_exp.h
reg_sub.o : reg_sub.c paths.h reg_exp.h

clean:
	rm *.o                        # change it to del *.o for OS/2

