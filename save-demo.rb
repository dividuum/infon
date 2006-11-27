require 'socket'
require 'zlib'

raise "#$0 <server> <demofile>" unless ARGV.size == 2
 
TCPSocket.open(ARGV[0], 1234) { |socket|
    
    class << socket
        alias_method  :orig_read, :read 
        attr_accessor :traffic
        attr_accessor :compress
        attr_accessor :zstream
        attr_accessor :buf
        attr_accessor :dump

        def read(x)
            if @compress
                while @buf.size < x
                    @buf << @zstream.inflate(orig_read(1))
                    @traffic += 1
                end
                ret = @buf[0...x]
                @buf = @buf[x..-1]
            else
                ret = orig_read(x)
                @traffic += x
            end
            @dump.write(ret) if @dump
            ret
        end
       
        def read8
            read(1)[0]
        end

        def read16
            ret = read8
            ret = ret & 0x7F | read8 << 7 if ret & 0x80 != 0 
            ret
        end

        def read32
            read(4).unpack("N")[0]
        end

        def readXX(len)
            read(len).unpack("A*")[0]
        end
    end

    socket.traffic  = 0
    socket.buf      = ""
    socket.compress = false
    socket.zstream  = Zlib::Inflate.new
    socket.dump     = File.new(ARGV[1], "wb+")

    loop do 
        len = socket.read8
        case type = socket.read8
        when 4:
            break
        when 32:
            socket.write("guiclient\n")
            puts  "welcome: %s"     % socket.read(len).delete("\n").strip
        when 254:
            puts "compression start"
            socket.compress = true
        else
            socket.readXX(len)
        end
        print "."
        STDOUT.flush
    end

    socket.dump.close
}
