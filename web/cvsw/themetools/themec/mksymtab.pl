# $Id: mksymtab.pl,v 1.1 2000/11/06 00:31:37 micahjd Exp $
#
# mksymtab.pl - convert the constant definitions in constants.h
#               into a symbol table to compile into the theme
#               compiler
#
# PicoGUI small and efficient client/server GUI
# Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
# 
# Contributors:
#
#
#
#

print "// Generated by mksymtab.pl from constants.h\n";

while (<>) {
    # Preserve the RCS tag just incase...
    if (/(\$[^\$]+\$)/) {
	print "// $1\n";

	# Start the rest of the file
	print "\n#include \"themec.h\"\n#include \"y.tab.h\"\n\nstruct symnode symboltab[] = {\n";

	next;
    }

    # We don't care about the symbol's actual value, let
    # the C compiler sort that out. Just get a list of 'em
    # so we can make a table

    next if (!/^\#define\s*(PG\S+)/);
    $n = $1;

    # All values can be used as-is as a numerical constant
    print "\t{NUMBER,\"$n\",$n},\n";

    # If this is a thobj, put it in with dotted lowercase notation
    if ($n =~ /^PGTH_O_(.*)/) {
	$_ = $1;
	tr/A-Z_/a-z./;
	print "\t{THOBJ,\"$_\",$n},\n";
    }

    # Same thing for properties
    if ($n =~ /^PGTH_P_(.*)/) {
	$_ = $1;
	tr/A-Z_/a-z./;
	print "\t{PROPERTY,\"$_\",$n},\n";
    }
}

print "\t{0,NULL,0}\n};\n\n/* The End */\n";

### The End ###





















