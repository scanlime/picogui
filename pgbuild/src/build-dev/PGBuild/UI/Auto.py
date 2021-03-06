""" PGBuild.UI.Auto

The 'auto' UI, that automatically picks a UI module in order of preference
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

import PGBuild.UI

# Try loading each UI in the catalog, transmogrify our Interface into the
# Interface of the first one that loads successfully.
for (name, description) in PGBuild.UI.catalog:
    try:
        mod = PGBuild.UI.find(name)
        break
    except:
        pass
Interface = mod.Interface
        
### The End ###
        
    
