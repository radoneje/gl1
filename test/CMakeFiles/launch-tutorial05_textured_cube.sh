#!/bin/sh
bindir=$(pwd)
cd /Users/denisshevchenko/Downloads/ogl-master/tutorial05_textured_cube/
export 

if test "x$1" = "x--debugger"; then
	shift
	if test "x" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		GDB_COMMAND-NOTFOUND -batch -command=$bindir/gdbscript  /Users/denisshevchenko/Downloads/ogl-master/test/tutorial05_textured_cube 
	else
		"/Users/denisshevchenko/Downloads/ogl-master/test/tutorial05_textured_cube"  
	fi
else
	"/Users/denisshevchenko/Downloads/ogl-master/test/tutorial05_textured_cube"  
fi
