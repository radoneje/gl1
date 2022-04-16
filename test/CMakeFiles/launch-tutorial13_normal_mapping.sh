#!/bin/sh
bindir=$(pwd)
cd /Users/denisshevchenko/Downloads/ogl-master/tutorial13_normal_mapping/
export 

if test "x$1" = "x--debugger"; then
	shift
	if test "x" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		GDB_COMMAND-NOTFOUND -batch -command=$bindir/gdbscript  /Users/denisshevchenko/Downloads/ogl-master/test/tutorial13_normal_mapping 
	else
		"/Users/denisshevchenko/Downloads/ogl-master/test/tutorial13_normal_mapping"  
	fi
else
	"/Users/denisshevchenko/Downloads/ogl-master/test/tutorial13_normal_mapping"  
fi
