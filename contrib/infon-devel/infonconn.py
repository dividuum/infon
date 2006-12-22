import gobject

import random
import os
import pwd
import asyncore
import socket
import errno
import re

STATE_PRESETUP = 0  # Nothing configured. Maybe not used.
STATE_OFFLINE = 1
STATE_CONNECTING = 2
STATE_CONNECTED = 3
STATE_DISCONNECTING = 4

DIR_NONE = 0
DIR_INCOMING = 1
DIR_OUTGOING = 2

class InfonConn(gobject.GObject):
    __gproperties__ = {
        'state' : (
            gobject.TYPE_INT,
            'state of the connection',
            '',
            STATE_PRESETUP,
            STATE_DISCONNECTING,
            STATE_OFFLINE,
            gobject.PARAM_READWRITE
        ),
    }

    __gsignals__ = { 
        'new_message' : (
            gobject.SIGNAL_RUN_LAST,
             gobject.TYPE_NONE,
            (gobject.TYPE_INT, gobject.TYPE_STRING,)
        ),
        'new_bt' : (
            gobject.SIGNAL_RUN_LAST,
             gobject.TYPE_NONE,
            (),
        ),
    }


    def __init__(self):
        gobject.GObject.__init__(self) 
        self.state = STATE_OFFLINE

        #self.hostname = "infon.dividuum.de"
        self.hostname = "localhost"
        self.port = "1234"
        self._username = pwd.getpwuid(os.getuid())[0]
        self.password = "pw%04d" % random.randrange(0,10000)

        self.playerid =  None

        self.socket = None
        self.outbuffer = ""
        self.codebuffer = ""
        self.btbuffer = ""
        self.waiting_code = None



    def do_get_property(self, property):
        if property.name == 'state':
            return self.state
        else:
             raise AttributeError, 'unknown property %s' % property.name

    def do_set_property(self, property, value):
        if property.name == 'state':
            self.state = value
        else:
             raise AttributeError, 'unknown property %s' % property.name

    def open_connection(self):
        assert not self.socket

        self.props.state = STATE_CONNECTING
        self.emit("new_message", DIR_NONE, "Connecting to %s:%s..." % (self.hostname,self.port))
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setblocking(False)
        try:
            self.socket.connect( (self.hostname, int(self.port)) )
        except socket.error, (x,s): 
            if x !=  errno.EINPROGRESS:
                self.emit("new_message", DIR_NONE, "failed: %s!" % s)
                self.props.state = STATE_OFFLINE
        gobject.io_add_watch(self.socket, gobject.IO_OUT, self.connected)

    def start_writing(self):
        def outgoing(socket, condition):
            if  len(self.codebuffer) > 0:
                sent = socket.send(self.codebuffer)
                self.emit("new_message", DIR_OUTGOING, "[your code...]\n")
                self.codebuffer = self.codebuffer[sent:]
            elif len(self.outbuffer) > 0:
                sent = socket.send(self.outbuffer)
                self.emit("new_message", DIR_OUTGOING, self.outbuffer[:sent])
                self.outbuffer = self.outbuffer[sent:]
            return len(self.codebuffer) > 0 or len(self.outbuffer) > 0 
        gobject.io_add_watch(self.socket, gobject.IO_OUT, outgoing)
        
    def connected(self,s,condition):
        self.emit("new_message", DIR_NONE, "Connected!")
        gobject.io_add_watch(s, gobject.IO_IN | gobject.IO_PRI, self.incoming)

    player_re = re.compile("joined. player (\d+) ")
    def incoming(self, s, condition):
        data = ""
        try:
            data = s.recv(8192)
        except socket.error, (x,s): 
            self.emit("new_message", DIR_NONE, "\nConnection Closed: %s!\n" % s)
            self.props.state = STATE_OFFLINE
            return False

        if data:
            self.emit("new_message", DIR_INCOMING, data)
            
            if self.state == STATE_CONNECTING:
                if data.find("Press <enter>") != -1:
                    self.send("\n")
                elif data.find("enter '?' for help") != -1:
                    self.send("j\n")
                elif data.find("press enter for new player") != -1:
                    self.send("\n")
                elif data.find("password for new player") != -1:
                    self.send("%s\n" % self.password)
                elif data.find("joined. player") != -1:
                    m = self.player_re.search(data)
                    assert m
                    self.playerid = int(m.group(1))
                    self.start_bting()
                    assert data.find("> ") != -1
                    self.send ("n\n")

            if self.state == STATE_CONNECTING or self.state == STATE_CONNECTED:
                if data.find("Player Name:") != -1:
                    self.send("%s\n" % self.username)
                    self.props.state = STATE_CONNECTED

            if self.state == STATE_CONNECTED:
                if data.find("enter your lua code") != -1:
                    assert self.waiting_code
                    self.codebuffer = self.waiting_code
                    self.waiting_code = None
                    self.start_writing()
                elif data.find("traceback:") != -1:
                    self.parse_backtrace(data)
                if data.find("> ") != -1:
                    self.start_writing()

        return True

    def upload(self, code):
        assert self.props.state == STATE_CONNECTED
        self.waiting_code = "%s\n.\n" % code
        self.send("b\n")

    def reload(self):
        assert self.props.state == STATE_CONNECTED
        self.send("r\n")

    def send(self, txt):
        self.outbuffer += txt
        self.start_writing()

    def set_username(self,username):
        changed = self._username != username
        self._username = username
        if changed and self.props.state == STATE_CONNECTED:
            self.send("n\n")

    
    def close_connection(self):
        assert self.props.state == STATE_CONNECTED
        self.send("q\n")
        self.props.state = STATE_DISCONNECTING
    

    ### BT Getting ###
    def start_bting(self):
        #self.emit("new_message", DIR_NONE,"Connecting to %s:%s..." % (self.hostname,self.port))
        self.btsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.btsocket.setblocking(False)
        try:
            self.btsocket.connect( (self.hostname, int(self.port)) )
        except socket.error, (x,s): 
            if x !=  errno.EINPROGRESS:
                self.emit("new_message", DIR_NONE, "BT socket failed: %s!" % s)
        gobject.io_add_watch(self.btsocket, gobject.IO_OUT, self.btconnected)

    def btconnected(self,s,condition):
        #self.emit("new_message", DIR_NONE, "Connected Backtrace-Getter")
        gobject.io_add_watch(s, gobject.IO_IN | gobject.IO_PRI, self.btincoming)
        
    def btincoming(self, s, condition):
        try:
            data = s.recv(8192)
        except socket.error, (x,s): 
            self.btconnected = False
            return False
        
        if data:
            if data.find("Press <enter>") != -1:
                self.btsend("\n")
            elif data.find("enter '?' for help") != -1:
                self.btsend("j\n")
            elif data.find("press enter for new player") != -1:
                self.btsend("%d\n" % self.playerid)
            elif data.find("password for player") != -1:
                self.btsend("%s\n" % self.password)
            elif data.find("joined. player ") != -1:
                gobject.timeout_add(200, self.get_backtrace);
                self.btconnected = True
            elif data.find("traceback:") != -1:
                    self.parse_backtrace(data)
        return True

    creature_re = re.compile("^<creature (\d+) ")
    line_re     = re.compile('from client \d+"]:(\d+):')
    def parse_backtrace(self,bt):
        lines = bt.splitlines()
        self.bt = []
        creature_num = None
        for line in lines:
            m = self.creature_re.match(line)
            if m:
                if creature_num:
                    self.bt.append( (creature_num, None) )
                creature_num = int(m.group(1))
            m = self.line_re.search(line)
            if m:
                if creature_num:
                    codeline = int(m.group(1)) - 1
                    self.bt.append( (creature_num, codeline) )
                    creature_num = None
        if creature_num:
            self.bt.append( (creature_num, None) )
        self.emit("new_bt")
    
    def btsend(self, txt):
        self.btbuffer += txt
        self.start_btwriting()
            
    def get_backtrace(self):
        if self.btconnected:
            self.btsend("i\n")
            self.start_btwriting()
            return True
        else:
            return False

    def start_btwriting(self):
        def btoutgoing(socket, condition):
            if  len(self.btbuffer) > 0:
                sent = socket.send(self.btbuffer)
                self.btbuffer = self.btbuffer[sent:]
            return len(self.btbuffer)
        gobject.io_add_watch(self.btsocket, gobject.IO_OUT, btoutgoing)
    
    ## Properties

    username = property( lambda s: s._username, set_username)

gobject.type_register(InfonConn)

# vim:ts=4:sw=4:sts=4:et
