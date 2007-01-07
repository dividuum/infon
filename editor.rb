require 'socket'

gfx = 0
TCPServer.open(0, 1234) do |srv|
    while client = srv.accept do
        client.write " " * 34
        client.write([4, 6, 40, 30, 5, 5].pack("c*"))
        while line = client.readline.chomp do
            case line
            when /D.,(.*),(.*)/
                client.write([4, 1, $1.to_i / 256, $2.to_i / 256, 0, gfx].pack("c*"))
            when /K(.*)/
                case $1.to_i
                when ?q: gfx = 0
                when ?w: gfx = 1
                when ?e: gfx = 2
                when ?r: gfx = 3
                when ?a: gfx = 4
                when ?s: gfx = 5
                when ?d: gfx = 6
                when ?f: gfx = 7
                when ?z: gfx = 8
                when ?x: gfx = 9
                when ?c: gfx =10
                end
            end
        end
    end
end
