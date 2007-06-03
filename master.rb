require 'socket'
require 'thread'
require 'webrick'
include WEBrick

http = HTTPServer.new(:Port => 1235)

udpsocket = UDPSocket.open
udpsocket.bind(nil, 1234)

servers  = {}
servers_mutex = Mutex.new

class Server
    attr_reader :clients, :players, :creatures

    def initialize(clients, players, creatures, name)
        @clients, @players, @creatures, @name = 
            clients, players, creatures, name
        @created = Time.now
    end

    def expired?
        @created + 240 < Time.now
    end

    def name
        @name.tr('^a-zA-Z0-9 ', ' ') 
    end

end

listener = Thread.start do 
    while true do
        begin
            input, source = udpsocket.recvfrom(128)
            raise 'invalid size' unless input[0] + 2 == input.size 
            raise 'invalid type' unless input[1] == 253
            addr = "%s:%d" % [source[3], source[1]] 
            servers_mutex.synchronize do 
                servers[addr] = Server.new(input[2], input[3], input[4], input[5..-1])
            end
        rescue => e
            p e
        end
    end
end

expire = Thread.start do 
    while true do 
        servers_mutex.synchronize do 
            servers.delete_if { |ip, server| server.expired? }
        end
        sleep 60
    end
end

http.mount_proc("/") { |req, res|
    res.body << <<-HTML
<html>
<script src='/servers.js'></script>
<body>
    <div id="infon_score" style='border:1px solid black; width: 300px;'>
        Loading Server List...
    </div>
<script>
    var servers = get_infon_servers();
    var html    = "<table width='100%'>";
    html += "<td><i>IP</i></td><td><i>Name</i></td><td><i>Clients</i></td></tr>";
    for (i = 0; i < servers.length; i++) {
        html += "<tr><td>" + servers[i].ip + "</td><td>" + 
                             servers[i].name + "</td><td>" + 
                             servers[i].clients + "</td>";
    }
    html += "</table>";
    document.getElementById("infon_score").innerHTML = html;
</script>
</body>
</html>
    HTML
    res['Content-Type'] = "text/html"
}

http.mount_proc("/servers.js") { |req, res|
    res.body << "function get_infon_servers() { var servers = new Array();\n"
    servers.sort_by { |ip, server| server.players }.each do |ip, server|
        res.body << "var s = new Object(); servers[servers.length] = s;\n"
        res.body << "s.ip = '%s'; s.name = '%s'\n" % [ip, server.name]
        res.body << "s.clients = %d; s.players = %d; s.creatures = %d;\n" %
                    [server.clients, server.players, server.creatures]
    end
    res.body << "return servers; }\n"
    res['Content-Type'] = "text/javascript"
}

trap("INT")  { http.shutdown }
trap("TERM") { http.shutdown }
http.start
listener.kill
