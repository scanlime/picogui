#!/usr/bin/env python

import PicoGUI, struct, os
from PicoGUI import constants

class TerminalPage:
    def __init__(self, parent, relation, app, position):
        self.tabpage = parent.addWidget('tabpage', relation)
	self._terminal = self.tabpage.addWidget('terminal', 'inside')
	self._termProcess = False
	self._app = app
	self._position = position
	if position == 0:
	    print "setting hotkey\n"
	    self.tabpage.hotkey = 'f1'

	try:
            import pty, fcntl, termios
            self._fcntl = fcntl
            self._termios = termios
        
            (pid, fd) = pty.fork()
            if pid == 0:
                os.execlp("sh", "sh", "--login")
            self._ptyfd = fd
            self._ptypid = pid

            self._fcntl.fcntl(self._ptyfd, self._fcntl.F_SETFL, os.O_NDELAY)
	    self._termProcess = True
	    self._app.link(self.terminalHandler, self._terminal, 'data')
	    self._app.link(self.terminalResizeHandler, self._terminal, 'resize')
	    self._app.link(self.tabClicked, self.tabpage, 'activate')
	except:
#	    self._terminal.write("The terminal isn't supported on this operating system.\n\r")
	    pass

    def terminalHandler(self, ev):
        os.write(self._ptyfd, ev.data)

    def terminalResizeHandler(self, ev):
        if ev.x > 0 and ev.y > 0:
	    self._fcntl.ioctl(self._ptyfd, self._termios.TIOCSWINSZ, struct.pack('4H', ev.y, ev.x, 0, 0))

    def terminalRead(self):
        if self._termProcess:
	    try:
	        self._terminal.write(os.read(self._ptyfd, 4096))
	    except OSError:
	        pass

    def tabClicked(self, ev):
        self._terminal.focus()

class App(PicoGUI.Application):
    def __init__(self):
        self._pages = []
	PicoGUI.Application.__init__(self, 'epterm')
	self._toolbar = self.addWidget('toolbar')

    def appendtab(self):
        try:
	    parent = self._pages[-1].tabpage
        except IndexError:
            parent = self._toolbar
	self._pages.append(TerminalPage(parent,'after', self, len(self._pages) - 1))
	self._pages[-1].tabpage.text = 'tab!'

    def update(self):
        import time
        self.eventPoll()
        for i in range(0,len(self._pages)):
	    self._pages[i].terminalRead()
	time.sleep(0.001)


f = App()
num_tabs = 4
for i in range(0,num_tabs):
    f.appendtab()
while(1):
    f.update()
