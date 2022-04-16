#!/bin/sh
bindir=$(pwd)
cd /Users/denisshevchenko/Downloads/ogl-master/misc05_picking/
export 

if test "x$1" = "x--debugger"; then
	shift
	if test "x" = "xYES"; then
		echo "r  " > $bindir/gdbscript
		echo "bt" >> $bindir/gdbscript
		GDB_COMMAND-NOTFOUND -batch -command=$bindir/gdbscript  /Users/denisshevchenko/Downloads/ogl-master/test/misc05_picking_BulletPhysics 
	else
		"/Users/denisshevchenko/Downloads/ogl-master/test/misc05_picking_BulletPhysics"  
	fi
else
	"/Users/denisshevchenko/Downloads/ogl-master/test/misc05_picking_BulletPhysics"  
fi
