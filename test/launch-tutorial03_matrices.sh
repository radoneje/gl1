#!/bin/sh
bindir=$(pwd)
cd /Users/denisshevchenko/Downloads/ogl-master/tutorial03_matrices/
export 

if test "x$1" = "x--debugger"; then
	shift
	if test "x" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		GDB_COMMAND-NOTFOUND -batch -command=$bindir/gdbscript  /Users/denisshevchenko/Downloads/ogl-master/test/tutorial03_matrices 
	else
		"/Users/denisshevchenko/Downloads/ogl-master/test/tutorial03_matrices"  
	fi
else
	"/Users/denisshevchenko/Downloads/ogl-master/test/tutorial03_matrices"  
fi
