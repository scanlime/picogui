""" PGBuild

The main package for the PGBuild configuration and build utility
"""
# 
# PicoGUI Build System
# Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
# 
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#  
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#  
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
# 
_svn_id = "$Id$"

import PGBuild.Version
import PGBuild.Platform
import SCons

# This section defines the name of the package, and the current version.
# The 'release' below should be set to a version number for releases,
# and left as None for development checkouts.

name = "PGBuild"
description = "configuration and build tool"
release = None
version = PGBuild.Version.determineVersion()
platform = PGBuild.Platform.determinePlatform()
sconsVersion = SCons.__version__

about = "%s %s\nVersion %s on %s" % (name, description, version, platform)

### The End ###
        
    
