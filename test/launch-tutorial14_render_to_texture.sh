#!/bin/sh
bindir=$(pwd)
cd /Users/denisshevchenko/Downloads/ogl-master/tutorial14_render_to_texture/
export 

if test "x$1" = "x--debugger"; then
	shift
	if test "x" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		GDB_COMMAND-NOTFOUND -batch -command=$bindir/gdbscript  /Users/denisshevchenko/Downloads/ogl-master/test/tutorial14_render_to_texture 
	else
		"/Users/denisshevchenko/Downloads/ogl-master/test/tutorial14_render_to_texture"  
	fi
else
	"/Users/denisshevchenko/Downloads/ogl-master/test/tutorial14_render_to_texture"  
fi
