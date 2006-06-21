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
    
    p socket.gets
    socket.write("g\n")
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
            puts  "%d, %d => %d (%d) " % [socket.read8, socket.read8, socket.read8, socket.read16]
        when 2:
            puts  "msg: %s  " % socket.read(len).unpack("A*")[0] 
        else
            puts  "???: #{type}"
        end
    end
}
