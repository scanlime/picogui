#!/usr/bin/env python
import sys
from Nifty import Frame, FileBuffer, ScratchBuffer

frame = Frame("This here is my text editor. Is it not nifty? Worship the text editor!")
for name in sys.argv[1:]:
    frame.open(FileBuffer(name))
if len(frame._pages) == 0:
    frame.open(ScratchBuffer('__scratch__', 'This buffer is for experimenting with Nifty'))

sys.exit(frame.run())

