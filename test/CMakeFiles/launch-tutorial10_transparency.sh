#!/bin/sh
bindir=$(pwd)
cd /Users/denisshevchenko/Downloads/ogl-master/tutorial10_transparency/
export 

if test "x$1" = "x--debugger"; then
	shift
	if test "x" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		GDB_COMMAND-NOTFOUND -batch -command=$bindir/gdbscript  /Users/denisshevchenko/Downloads/ogl-master/test/tutorial10_transparency 
	else
		"/Users/denisshevchenko/Downloads/ogl-master/test/tutorial10_transparency"  
	fi
else
	"/Users/denisshevchenko/Downloads/ogl-master/test/tutorial10_transparency"  
fi
