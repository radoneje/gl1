#!/bin/sh
bindir=$(pwd)
cd /Users/denisshevchenko/Downloads/ogl-master/tutorial08_basic_shading/
export 

if test "x$1" = "x--debugger"; then
	shift
	if test "x" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		GDB_COMMAND-NOTFOUND -batch -command=$bindir/gdbscript  /Users/denisshevchenko/Downloads/ogl-master/test/tutorial08_basic_shading 
	else
		"/Users/denisshevchenko/Downloads/ogl-master/test/tutorial08_basic_shading"  
	fi
else
	"/Users/denisshevchenko/Downloads/ogl-master/test/tutorial08_basic_shading"  
fi
