# $Id: console.py,v 1.2 2002/11/26 23:03:04 micahjd Exp $
#
# console.py - Implement a popup python console
#
# Copyright (C) 2002 Micah Dowty and David Trowbridge
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

import PicoGUI, sys
from code import InteractiveConsole


class Console(InteractiveConsole):
    def __init__(self, game):
        self.game = game
        self.app = game.app
        
        # Load our widget template, importing widgets from it
        self.template = self.app.newTemplate(open("data/console.wt").read())
        self.inst = self.template.instantiate([
            'PythonConsole',
            'PythonCommand',
            'PythonPrompt',
            'ConsoleToggle',
            ])
        
        # Redirect stdout and stderr to our console
        self.savedStdout = sys.stdout
        self.savedStderr = sys.stderr
        sys.stdout = self.inst.PythonConsole
        sys.stderr = self.inst.PythonConsole
        
        locals = {
            "__name__":   "__console__",
            "__doc__":    None,
            "PicoGUI":    PicoGUI,
            "game":       game,
            }
        InteractiveConsole.__init__(self,locals)

        try:
            sys.ps1
        except AttributeError:
            sys.ps1 = ">>> "
        try:
            sys.ps2
        except AttributeError:
            sys.ps2 = "... "

        self.prompt = sys.ps1
        self.inst.PythonPrompt.text = self.prompt
        self.app.link(self.enterLine, self.inst.PythonCommand, 'activate')

        print "Python %s on %s\n(JetEngine shell, See game.__dict__ for useful variables)\n" %\
              (sys.version, sys.platform)

    def enterLine(self, ev, widget):
        line = widget.text[:]
        print self.prompt + line
        if self.push(line):
            self.prompt = sys.ps2
        else:
            self.prompt = sys.ps1
        self.inst.PythonPrompt.text = self.prompt
        widget.text = ''
        
    def destroy(self):
        sys.stdout = self.savedStdout
        sys.stderr = self.savedStderr
        self.template.destroy()
