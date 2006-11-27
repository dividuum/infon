
import gtk
import pango

class ConnView(gtk.TextView):
    def __init__(self,*args):
        gtk.TextView.__init__(self,*args)

        font_desc = pango.FontDescription('monospace 10')
        if font_desc:
            self.modify_font(font_desc)

        self.props.editable = False
        self.props.cursor_visible = False

        self.props.buffer.create_tag("system", foreground="#555")
        self.props.buffer.create_tag("incoming", foreground="blue")
        self.props.buffer.create_tag("outgoing", foreground="red")

        self.connect_after("realize",ConnView.realized)
    
    def realized(self):
        end = self.props.buffer.get_end_iter()
        self.scroll_to_iter(end,0.1)

    def append_system(self, text):
        buffer = self.props.buffer
        end = buffer.get_end_iter()
        buffer.insert_with_tags_by_name(end, text, "system")
        if self.flags() & gtk.REALIZED:
            self.scroll_mark_onscreen(buffer.get_insert())
    
    def append_incoming(self, text):
        buffer = self.props.buffer
        end = buffer.get_end_iter()
        buffer.insert_with_tags_by_name(end, text, "incoming")
        if self.flags() & gtk.REALIZED:
            self.scroll_mark_onscreen(buffer.get_insert())
    
    def append_outgoing(self, text):
        buffer = self.props.buffer
        end = buffer.get_end_iter()
        buffer.insert_with_tags_by_name(end, text, "outgoing")
        if self.flags() & gtk.REALIZED:
            self.scroll_mark_onscreen(buffer.get_insert())

# vim:ts=4:sw=4:sts=4:et
