from PicoGUI import Widget
from Nifty.Workspace import Workspace
import persistence

class Viewer(Workspace):
    widget_type = 'box'
    
    def open(self, frame, page, buffer):
        self.fields = {}
        self._last = self.addWidget('Label', 'inside')
        self._last.text = 'Poing %s' % buffer.title()
        Workspace.open(self, frame, page, buffer)
        if hasattr(buffer, 'addField'):
            b = self._last.addWidget('Box')
            b.side = 'bottom'
            self._add_name = b.addWidget('Field', 'inside')
            self._add_name.side = 'left'
            self._add_name.size = 10
            self._add_name.sizemode = 'percent'
            self._add_value = self._add_name.addWidget('Field')
            self._add_value.side = 'all'
            button = self._add_name.addWidget('Button')
            button.side = 'right'
            button.text = 'add'
            self.frame.link(self._do_add, self._add_name, 'activate')
            self.frame.link(self._do_add, self._add_value, 'activate')
            self.frame.link(self._do_add, button, 'activate')

    def _add_field(self, field):
        if self.fields.has_key(field.name):
            return
        self._last = self._last.addWidget('Box')
        label = self._last.addWidget('Label', 'inside')
        label.text = field.name
        label.side = 'left'
        control = label.addWidget(field.widget)
        self.fields[field.name] = control
        self.frame.link(self._changed, control, 'changed')
        if hasattr(self.buffer, 'delField'):
            button = label.addWidget('Button')
            button.side = 'right'
            button.text = 'del'
            self.frame.link(self._do_del, button, 'activate')

    def _del_field(self, name):
        control = self.fields[name]
        del self.fields[name]
        box = control.parent.parent
        if box is self._last:
            self._last = box.parent
        else:
            # tricky - we don't want to leave a 'parent' attribute dangling
            other = self._last
            while other.parent is not box:
                other = other.parent
            other.parent = box.parent
        # delete the control, so that it's out of the event registry
        self.frame.delWidget(control)
        # and now the box (therefore the label and button too)
        self.frame.delWidget(box)

    def _do_add(self, ev):
        self.buffer.addField(self._add_name.text, self._add_value.text)
        self._add_name.text = ''
        self._add_value.text = ''
        self.update_view()

    def _do_del(self, ev):
        name = ev.widget.parent.text
        self.buffer.delField(name)
        self.update_view()

    def _changed(self, ev):
        self.frame.send(self, 'changed')

    def update_view(self):
        updated = []
        for field in self.buffer.Schema():
            self._add_field(field)
            self.fields[field.name].text = self.buffer.getField(field.name)
            updated.append(field.name)
        for name in self.fields.keys():
            if name not in updated:
                self._del_field(name)

    def update_buffer(self):
        for field in self.buffer.Schema():
            self.buffer.setField(field.name, self.fields[field.name].text)
