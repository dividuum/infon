#!/usr/bin/python
# encoding:utf8

#     Infon-Devel
#     Copyright (C) 2006 Joachim Breitner
#
#     This program is free software; you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation; either version 2 of the License, or
#     (at your option) any later version.
#
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with this program; if not, write to the Free Software

PREFIX = './'

import pygtk
pygtk.require('2.0')
import gtk
import gobject
import gtksourceview2

import os
import sys

if not PREFIX in sys.path:
    sys.path.insert(0, PREFIX)


import connview
import infonconn


class ConnDialog(gtk.Dialog):

    def __init__(self,window):
        gtk.Dialog.__init__(self,
            "Connect to Infon Server",
            window,
            gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT,    
            (
                gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                gtk.STOCK_OK,     gtk.RESPONSE_OK,
            )
        )
        self.e_hostname = gtk.Entry()
        self.e_port = gtk.Entry()
        self.e_username = gtk.Entry()
        self.e_password = gtk.Entry()


        self.vbox.add(gtk.Label("Server"))
        self.vbox.add(self.e_hostname)
        self.vbox.add(gtk.Label("Port"))
        self.vbox.add(self.e_port)
        self.vbox.add(gtk.Label("Username"))
        self.vbox.add(self.e_username)
        self.vbox.add(gtk.Label("Password"))
        self.vbox.add(self.e_password)

        self.vbox.show_all()

    hostname = property(
        lambda self: self.e_hostname.get_text(),
        lambda self,v: self.e_hostname.set_text(v))
    port = property(
        lambda self: self.e_port.get_text(),
        lambda self,v: self.e_port.set_text(v))
    username = property(
        lambda self: self.e_username.get_text(),
        lambda self,v: self.e_username.set_text(v))
    password = property(
        lambda self: self.e_password.get_text(),
        lambda self,v: self.e_password.set_text(v))
        
    def run(self):
        response = gtk.Dialog.run(self)
        self.hide()
        return response

    def run_name_only(self):
        self.e_hostname.set_sensitive(False)
        self.e_port.set_sensitive(False)
        self.e_password.set_sensitive(False)
        response = gtk.Dialog.run(self)
        self.hide()
        self.e_hostname.set_sensitive(True)
        self.e_port.set_sensitive(True)
        self.e_password.set_sensitive(True)
        return response

class InfonDevel:

    def new(self,action):
        # Check for possible changes!

        self.bugbuf.begin_not_undoable_action()

        self.bugbuf.set_text("")
        self.filename = None
        self.bugbuf.end_not_undoable_action()

        self.bugbuf.set_modified(False)
        self.bugbuf.place_cursor(self.bugbuf.get_start_iter())

    def open(self, action):
        chooser = gtk.FileChooserDialog('Open bug...', None,
                                    gtk.FILE_CHOOSER_ACTION_OPEN,
                                    (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                    gtk.STOCK_OPEN, gtk.RESPONSE_OK))
        response = chooser.run()
        if response == gtk.RESPONSE_OK:
            filename = chooser.get_filename()
            if filename:
                self.open_file(filename)
        chooser.destroy()

    def open_file(self,filename):
        if os.path.isabs(filename):
            path = filename
        else:
            path = os.path.abspath(filename)


        try:
            txt = open(path).read()
        except:
            return False

        self.bugbuf.begin_not_undoable_action()
        self.bugbuf.set_text(txt)
        self.bugbuf.end_not_undoable_action()
        
        self.filename = path

        self.bugbuf.set_modified(False)
        self.bugbuf.place_cursor(self.bugbuf.get_start_iter())

    def save(self,action):
        if not self.filename:
            self.save_as(action)
        else:
            if os.path.isabs(self.filename):
                path = self.filename
            else:
                path = os.path.abspath(self.filename)
            start = self.bugbuf.get_start_iter()
            end = self.bugbuf.get_end_iter()
            open(path,"w").write(self.bugbuf.get_text(start,end))
            self.bugbuf.set_modified(False)

    def save_as(self,action):
        chooser = gtk.FileChooserDialog('Save bug as...', None,
                                    gtk.FILE_CHOOSER_ACTION_SAVE,
                                    (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                    gtk.STOCK_SAVE, gtk.RESPONSE_OK))
        if self.filename:
            chooser.set_current_name(self.filename)
        response = chooser.run()
        if response == gtk.RESPONSE_OK:
            filename = chooser.get_filename()
            if filename:
                self.filename = filename
                self.save(action)
        chooser.destroy()

    def server_settings(self,action):
        self.conndialog.hostname = self.conn.hostname
        self.conndialog.port = self.conn.port
        self.conndialog.username = self.conn.username
        self.conndialog.password = self.conn.password
        if self.conndialog.run() == gtk.RESPONSE_OK:
            self.conn.hostname = self.conndialog.hostname
            self.conn.port = self.conndialog.port
            self.conn.username = self.conndialog.username
            self.conn.password = self.conndialog.password

    def change_name(self,action):
        self.conndialog.hostname = self.conn.hostname
        self.conndialog.port = self.conn.port
        self.conndialog.username = self.conn.username
        self.conndialog.password = self.conn.password
        if self.conndialog.run_name_only() == gtk.RESPONSE_OK:
            self.conn.username = self.conndialog.username

    def commit_code(self, action):
        start = self.bugbuf.get_start_iter()
        end = self.bugbuf.get_end_iter()
        self.conn.upload(self.bugbuf.get_text(start,end))
            
    def connect(self, action):
        self.conn.open_connection()
    def disconnect(self, action):
        self.conn.close_connection()
        

    def info(self,action):
        dialog = gtk.AboutDialog()
        dialog.set_authors(["Joachim Breitner <mail@joachim-breitner.de>"])
        dialog.set_license("GPL")
        dialog.set_name("Infon Devel")
        dialog.set_website("http://infon.dividuum.de/")
        dialog.set_website_label("http://infon.dividuum.de/")
#        dialog.set_version("0.0")
        dialog.run()
        dialog.destroy()

    def delete_event(self, widget, event, data=None):
        # Change FALSE to TRUE and the main window will not be destroyed
        # with a "delete_event".
        return False

    def setup_menu(self):
        actions = gtk.ActionGroup("menu")
        actions.add_actions([
            ('FileMenu', None, '_File'),
            ('new',  gtk.STOCK_NEW, '_New', None, 'New Bug', self.new),
            ('openbug',  gtk.STOCK_OPEN, '_Open Bug...', None, 'Open a new bug', self.open),
            ('savebug',  gtk.STOCK_SAVE, '_Save Bug', None, 'Save the current bug',self.save),
            ('savebugas',  gtk.STOCK_SAVE_AS, 'S_ave Bug As...', None, 'Save the current bug',self.save_as),
            ('quit', gtk.STOCK_QUIT, '_Quit', None, 'Quit the program', self.destroy),

            ('GameMenu', None, '_Game'),

            ('ViewMenu', None, '_View'),
            ('ChooseServer',  gtk.STOCK_NETWORK, 'Choose _Server', None, 'Choose an Infon Server',
                self.server_settings),
            ('ChangeName',  gtk.STOCK_EDIT, 'Change _Name', None, 'Change your Player Name',
                self.change_name),
            ('ChangeColor',  gtk.STOCK_SELECT_COLOR, 'Change C_olor', None, 'Change your Player’s Color', lambda a: None),
            ('CommitBug',  gtk.STOCK_APPLY, 'Commit _Bug', None, 'Commit your code to the server',
                self.commit_code),
            ('RestartBug',  gtk.STOCK_REFRESH, '_Restart Bug', None, 'Restart the your bug’s main loops',
                lambda a: self.conn.reload()),
            ('Connect', gtk.STOCK_CONNECT, '_Connect to Server', None, 'Connect to Server',
                            self.connect),
            ('Disconnect', gtk.STOCK_DISCONNECT, '_Disconnect from Server', None, 'Disconnect from Server',
                            self.disconnect),

            ('HelpMenu', None, '_Help'),
            ('info', gtk.STOCK_INFO, '_Info', None, 'Information about this program', self.info),
        ])
        actions.add_toggle_actions([
            ('ShowNumbers', None, 'Show _Line Numbers', None, 'Toggle visibility of line numbers in the left margin', 
                            lambda a, s: s.set_show_line_numbers(a.get_active())),
            ('ShowMarkers', None, 'Show _Markers', None, 'Toggle visibility of markers in the left margin',
                            lambda a, s: s.set_show_line_marks(a.get_active())),
            ('AutoIndent', None, 'Enable _Auto Indent', None, 'Toggle automatic auto indentation of text', 
                            lambda a, s: s.set_auto_indent(a.get_active())),
            #('InsertSpaces', None, 'Insert _Spaces Instead of Tabs', None, 'Whether to insert space characters when inserting tabulations', 
            #                lambda a, s: s.set_insert_spaces_instead_of_tabs(a.get_active())),
        ],self.bugview)

        self.ui = gtk.UIManager()
        self.ui.add_ui_from_string('''
         <ui>
          <menubar name="MenuBar">
            <menu action="FileMenu">
              <menuitem action="new" />
              <menuitem action="openbug" />
              <separator />
              <menuitem action="savebug" />
              <menuitem action="savebugas" />
              <separator />
              <menuitem action="quit" />
            </menu>
            <menu action="GameMenu">
              <menuitem action="ChooseServer"/>
              <menuitem action="ChangeName"/>
<!--               <menuitem action="ChangeColor"/> -->
              <separator />
              <menuitem action="Connect"/>
              <menuitem action="Disconnect"/>
              <separator />
              <menuitem action="CommitBug"/>
              <menuitem action="RestartBug"/>
            </menu>
            <menu action='ViewMenu'>
              <menuitem action='ShowNumbers'/>
              <menuitem action='ShowMarkers'/>
              <menuitem action='AutoIndent'/>
            <!--  <menuitem action='InsertSpaces'/> -->
              <separator/>
            <!-- <menu action='TabsWidth'>
                <menuitem action='TabsWidth4'/>
                <menuitem action='TabsWidth6'/>
                <menuitem action='TabsWidth8'/>
                <menuitem action='TabsWidth10'/>
                <menuitem action='TabsWidth12'/>
              </menu> -->
            </menu>
            <menu action="HelpMenu" position="bot">
              <menuitem action="info" />
            </menu>
          </menubar>
          <toolbar name="ToolBar">
            <toolitem action="new" />
              <toolitem action="openbug" />
              <toolitem action="savebug" />
              <toolitem action="savebugas" />
              <separator />
<!--              <toolitem action="quit" /> -->
              <toolitem action="ChooseServer"/>
              <toolitem action="ChangeName"/>
<!--               <toolitem action="ChangeColor"/> -->
              <separator />
              <toolitem action="Connect"/>
              <toolitem action="Disconnect"/>
              <separator />
              <toolitem action="CommitBug"/>
              <toolitem action="RestartBug"/>
          </toolbar>
        </ui>''')

        action = actions.get_action('ShowNumbers')
        action.set_active(self.bugview.get_show_line_numbers())
        action = actions.get_action('ShowMarkers')
        action.set_active(self.bugview.get_show_line_marks())
        action = actions.get_action('AutoIndent')
        action.set_active(self.bugview.get_auto_indent())
        #action = actions.get_action('InsertSpaces')
        #action.set_active(self.bugview.get_insert_spaces_instead_of_tabs())
        #action = actions.get_action('TabsWidth%d' % self.bugview.get_tabs_width())
        #if action:
        #    action.set_active(True)


        self.ui.insert_action_group(actions,0)

    def setup_bugview(self):
        self.bugbuf = gtksourceview2.Buffer()
        lm = gtksourceview2.LanguageManager()
        lang = lm.guess_language(None, "text/x-lua")
        self.bugbuf.set_highlight_syntax(True)
        self.bugbuf.set_language(lang)

        def set_bt_markers(blubb):
            begin, end = self.bugbuf.get_bounds()
            self.bugbuf.remove_source_marks(begin, end)

            for creature, line in self.conn.bt:
                if line:
                    self.bugbuf.create_source_mark(None, "bt", line)
        self.conn.connect("new_bt",set_bt_markers)

        self.bugview = gtksourceview2.View(self.bugbuf)
        self.bugview.props.show_line_numbers = True
        self.bugview.props.show_line_marks = True
        self.bugview.props.auto_indent = True
        #self.bugview.props.insert_spaces_instead_of_tabs = True

        icon = gtk.gdk.pixbuf_new_from_file(PREFIX + 'marker.png')
        self.bugview.set_mark_category_pixbuf("bt",icon)

    def setup_connview(self):
        self.connview = connview.ConnView()
        self.connview.append_system('Welcome to InfonDevel\n')



        def new_message(conn,type,msg):
            if type == infonconn.DIR_NONE:
                self.connview.append_system(msg)
            if type == infonconn.DIR_INCOMING:
                self.connview.append_incoming(msg)
            if type == infonconn.DIR_OUTGOING:
                self.connview.append_outgoing(msg)
                
        self.conn.connect("new_message",new_message)

        #self.connview.append_incoming('Incoming\n')
        #self.connview.append_outgoing('Outgoing\n')
        #self.connview.append_outgoing('Outgoing\n')
        #self.connview.append_outgoing('Outgoing\n')

    def setup_statusbar(self):
        self.status = gtk.Statusbar()

    def setup_conn(self):
        self.conn = infonconn.InfonConn()

    def setup_title(self):
        def set_title():
            title = ''
            title += "Infon Devel"
            if self.bugbuf.get_modified():
                title += " ⋆ "
            else:
                title += " – "
            if self.filename:
                title +=  self.filename
            else:
                title += "[unnamed]"
            if self.bugbuf.get_modified():
                title += " ⋆ "
            else:
                title += " – "
            state = self.conn.props.state
            if   state == infonconn.STATE_PRESETUP:
                title += "[no server specified]"
            elif state == infonconn.STATE_OFFLINE:
                title += "[offline]"
            elif state == infonconn.STATE_CONNECTING:
                title += "%s@%s:%s [connecting]" % (self.conn.username, self.conn.hostname, self.conn.port)
            elif state == infonconn.STATE_CONNECTED:
                title += "%s@%s:%s" % (self.conn.username, self.conn.hostname, self.conn.port)
            elif state == infonconn.STATE_DISCONNECTING:
                title += "%s@%s:%s [disconnecting]" % (self.conn.username, self.conn.hostname, self.conn.port)
                
            self.window.props.title = title

        self.bugbuf.connect("modified-changed", lambda b: set_title())
        self.conn.connect("notify::state", lambda c,p: set_title())
        set_title()

    def setup_toolbarstates(self):
        actions = self.ui.get_action_groups()[0]

        def statebased():
            state = self.conn.props.state
            online = state == infonconn.STATE_CONNECTED
            actions.get_action('ChangeName').set_sensitive(online)
            actions.get_action('ChangeColor').set_sensitive(online)
            actions.get_action('CommitBug').set_sensitive(online)
            actions.get_action('RestartBug').set_sensitive(online)

            actions.get_action('Connect').set_sensitive(not online)
            actions.get_action('Disconnect').set_sensitive(online)

            actions.get_action('ChooseServer').set_sensitive(
                state == infonconn.STATE_OFFLINE or state == infonconn.STATE_PRESETUP
            )
       
        self.conn.connect("notify::state", lambda c,p: statebased())
        statebased()

    def __init__(self):
        self.filename = None

        self.setup_conn()

        self.setup_bugview()
        self.setup_connview()
        self.setup_menu()
        self.setup_statusbar()


        pane = gtk.VPaned()
        frame = gtk.ScrolledWindow()
        frame.add(self.bugview)
        frame.props.shadow_type = gtk.SHADOW_IN
        frame.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_ALWAYS)
        pane.pack1(frame, True)
        frame = gtk.ScrolledWindow()
        frame.add(self.connview)
        frame.props.shadow_type = gtk.SHADOW_IN
        frame.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_ALWAYS)
        pane.pack2(frame, False)
        pane.props.position = 300
        
        vbox1 = gtk.VBox()
        vbox1.pack_start(self.ui.get_widget('/MenuBar'), expand=False)
        vbox1.pack_start(self.ui.get_widget('/ToolBar'), expand=False)
        vbox1.pack_start(pane)
        vbox1.pack_start(self.status, expand=False)

        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        self.window.set_default_size(700,432) # Golden Ratio :-)
        self.window.props.border_width = 0
        self.window.add_accel_group(self.ui.get_accel_group())
        #self.window.set_icon(gtk.gdk.pixbuf_new_from_file('icon.png'))
        self.window.connect("delete_event", self.delete_event)
        self.window.connect("destroy", self.destroy)
        self.window.add(vbox1)

        self.conndialog = ConnDialog(self.window)

        self.setup_title()
        self.setup_toolbarstates()

        self.bugview.grab_focus()

        self.window.show_all()

    def main(self):
        gtk.main()

    def destroy(self, widget, data=None):
        gtk.main_quit()

if __name__ == "__main__":
    InfonDevel().main()

# vim:ts=4:sw=4:sts=4:et

