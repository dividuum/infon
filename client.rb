require 'socket'
TCPSocket.open('localhost', 1234) { |socket|
    class << socket
        alias_method :orig_read, :read 
        attr_reader :traffic

        def read(x)
            @traffic = 0 unless @traffic
            @traffic += x
            orig_read(x)
        end
       
        def read8
            read(1)[0]
        end

        def read16
            read(2).unpack("n")[0]
        end

        def read32
            read(4).unpack("N")[0]
        end
    end
    
    loop do 
        len = socket.read8
        print "len=%3d " % len
        case type = socket.read8
        when 0:
            print "player upd: "
            print "pno=%d "   % socket.read8
            print "mask=%d "  % mask = socket.read8
            print "alive=%s " % [socket.read8.zero? ? "quit" : "joined"]  if mask &  1 != 0
            print "name=%s  " % socket.read(socket.read8).unpack("A*")[0] if mask &  2 != 0
            print "color=%d " % socket.read8                              if mask &  4 != 0
            print "cpu=%d "   % socket.read8                              if mask &  8 != 0
            print "score=%d " % (socket.read16 - 500)                     if mask & 16 != 0
            puts
        when 1:
            puts  "%d, %d => %d (%d) " % [socket.read8, socket.read8, socket.read8, socket.read8]
        when 2:
            puts  "msg: %s  " % socket.read(len).unpack("A*")[0] 
        when 3: 
            print "creature upd: "
            print "cno=%d "         % socket.read16
            print "mask=%d "        % mask = socket.read8
            print "alive=%s "       % [socket.read8 == 0xFF ? "dead" : "spawned"]   if mask &  1 != 0
            print "pos=%d,%d,%d "   % [socket.read16, socket.read16, socket.read8]  if mask &  2 != 0
            print "type=%d "        % socket.read8                                  if mask &  4 != 0
            if mask &  8 != 0
                fh = socket.read8
                print "food=%d, health=%d " % [ fh >> 4, fh & 0x0F ]
            end
            print "state=%d "       % socket.read8                                  if mask & 16 != 0
            print "target=%d "      % socket.read16                                 if mask & 32 != 0
            print "message=%s "     % socket.read(socket.read8).unpack("A*")[0]     if mask & 64 != 0
            puts
        when 4:
            puts  "quit msg: %s  "  % socket.read(len).unpack("A*")[0] 
            break
        when 5:
            puts  "king: %d  "      % socket.read8
        when 32:
            socket.write("guiclient\n")
            puts  "welcome: %s"     % socket.read(len).delete("\n").strip
        else
            puts  "???: #{type}"
        end
    end
}
