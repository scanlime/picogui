from Buffer import TextBuffer
import os

class FileBuffer(TextBuffer):
    "Represents a file"

    def __init__(self, path=''):
        self.path = os.path.expanduser(os.path.expandvars(path))
        self.basename = os.path.basename(self.path)
        self.changed = False
        if os.path.exists(self.path):
            f = file(self.path, 'r')
            text=f.read()
            f.close()
        else:
            print 'New file'
            text = ''
        TextBuffer.__init__(self, name=self.basename, text=text)

    def notify_changed(self, ev):
        TextBuffer.notify_changed(self, ev)
        self.changed = True
        self.change_name('*' + self.basename)

    def save(self):
        f = file(self.path, 'w')
        f.write(self.text)
        f.close()
        print 'buffer %r saved' % self.basename
        self.changed = False
        self.change_name(self.basename)
