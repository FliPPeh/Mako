CC=clang

# CFLAGS that should be shared between modules and the main application.
# Necessary to make include paths known to everyone.
PRJCFLAGS=-I$(PWD)/src/ -I$(PWD)/lib/libutil/include -Wall -g
