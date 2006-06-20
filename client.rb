require 'socket'
TCPSocket.open('localhost', 1234) { |socket|
    p socket.gets
    socket.write("g\n")
    while foo = socket.read(1) 
        case foo[0]
        when 0:
            print "player static: "
            print "pno=%d " % socket.read(1)[0]
            print "name=%s " % socket.read(32).unpack("Z*")[0]
            puts  "color=%d" % socket.read(1)[0]
        when 1:
            print "player round: "
            print "pno=%d " % socket.read(1)[0]
            print "score=%d " % socket.read(4).unpack("N")[0]
            puts "cycles=%d" % socket.read(4).unpack("N")[0]
        when 2:
            print "player join/leave: "
            print "pno=%d " % socket.read(1)[0]
            puts "op=%s" % (socket.read(1)[0] == 0 ? "leave" : "join")
        when 3:
            print "king update: "
            puts "king=%d " % socket.read(1)[0]
        else
            print "???"
        end
    end
}
