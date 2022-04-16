#!/bin/sh
bindir=$(pwd)
cd /Users/denisshevchenko/Downloads/ogl-master/playground/
export 

if test "x$1" = "x--debugger"; then
	shift
	if test "x" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		GDB_COMMAND-NOTFOUND -batch -command=$bindir/gdbscript  /Users/denisshevchenko/Downloads/ogl-master/test/playground 
	else
		"/Users/denisshevchenko/Downloads/ogl-master/test/playground"  
	fi
else
	"/Users/denisshevchenko/Downloads/ogl-master/test/playground"  
fi
