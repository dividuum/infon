require 'socket'
require 'zlib'
require 'stringio'

class Tile
    attr_accessor :food, :type, :gfx
    def initialize(type, gfx, food)
        @food, @type, @gfx = food, type, gfx
    end
end

class Creature
    attr_accessor :type, :state, :food, :health, :target, :message, :speed, :x, :y, :id, :player, :vm_id
    def initialize(player, vm_id, x, y)
        @player, @vm_id, @x, @y = player, vm_id, x, y
    end
end

class Player
    attr_accessor :name, :color, :cpu, :score, :id
end

class InfonDemo
    attr_reader :players, :world, :creatures, :time, :world_w, :world_h, :koth_x, :koth_y
    def initialize(file)
        #@file = StringIO.new(File.read(file))
        @file = File.open(file, "rb")
        class << @file
            alias_method  :orig_read, :read 
            attr_accessor :traffic
            attr_accessor :compress
            attr_accessor :zstream
            attr_accessor :buf

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
        @file.traffic  = 0
        @file.buf      = ""
        @file.compress = false
        @file.zstream  = Zlib::Inflate.new

        @world     = Hash.new
        @time      = 0
        @creatures = Hash.new
        @players   = Hash.new
    end

    def tick 
        loop do 
            len = @file.read8
            case type = @file.read8
            when 0:
                pno  = @file.read8
                mask = @file.read8
                if mask &  1 != 0
                    if @file.read8 == 0
                        @players.delete(pno)
                    else
                        player = @players[pno] = Player.new
                        player.id = pno
                    end
                else
                    player = @players[pno]
                end
                if mask &  2 != 0
                    player.name = @file.readXX(@file.read8)
                end
                if mask &  4 != 0
                    player.color = @file.read8                              
                end
                if mask &  8 != 0
                    player.cpu = @file.read8
                end
                if mask & 16 != 0
                    player.score = @file.read16 - 500
                end
            when 1:
                pos = [@file.read8, @file.read8]
                food_type, gfx = @file.read8, @file.read8
                @world[pos] = Tile.new(food_type & 0xF0 >> 4, gfx, food_type & 0xF)
            when 2:
                msg = @file.readXX(len)
            when 3: 
                cno      = @file.read16
                mask     = @file.read8
                if mask &  1 != 0            
                    pno = @file.read8
                    if pno == 0xFF
                        @creatures.delete(cno)
                    else
                        creature = @creatures[cno] = Creature.new(pno, @file.read16, @file.read16, @file.read16)
                        creature.id = cno
                    end
                else
                    creature = @creatures[cno]
                end                                
                if mask &  2 != 0
                    creature.type = @file.read8 
                end
                if mask &  4 != 0
                    fh = @file.read8
                    creature.food   = fh >> 4
                    creature.health = fh & 0x0F
                end
                if mask &  8 != 0
                    creature.state = @file.read8
                end
                if mask & 16 != 0
                    dx, dy = @file.read16, @file.read16
                    x = ((dx & 1) == 1 ? -1 : 1) * (dx >> 1)
                    y = ((dy & 1) == 1 ? -1 : 1) * (dy >> 1)
                    creature.x += x
                    creature.y += y
                end
                if mask & 32 != 0
                    creature.target = @file.read16
                end
                if mask & 64 != 0
                    creature.message = @file.readXX(@file.read8)       
                end
                if mask & 128 != 0
                    creature.speed = @file.read8
                end
            when 4:
                puts @file.readXX(len) #quit 
                return false
            when 5:
                king = @file.read8
            when 6:
                @world_w, @world_h, @koth_x, @koth_y = @file.read8, @file.read8, @file.read8, @file.read8
            when 7: 
                smileno = @file.read16
            when 8: 
                @time = @file.read32
            when 9: 
                @time += @file.read8
                return true
            when 10:
                intermission = @file.readXX(len)
            when 32:
                welcome = @file.read(len).delete("\n").strip
            when 254:
                if @file.compress
                    puts "This Demo is compressed twice"
                    exit(1)
                end
                @file.compress = true
            when 255:
                version = @file.read8
                # puts  "server protocol version %d" % @file.read8
            else
                raise "???: unknown packet type #{type}"
                @file.readXX(len)
            end
        end
    end

    def dump_world
        @world_w.times do |x|
            @world_h.times do |y|
                case @world[[x,y]].type
                when 0: print "#"
                when 1: print " "
                when 2: print "~"
                else    print "?"
                end
            end
            puts
        end
    end
end

if $0 == __FILE__
    infon = InfonDemo.new(ARGV[0])
    while infon.tick 
        print "%2d:%02d " % [infon.time / 1000 / 60, infon.time / 1000 % 60]
        #puts "%d %d" % [infon.players.size, infon.creatures.size]
        20.times do |i|
            if creature = infon.creatures[i]
                print "%d %d " % [creature.x, creature.y]
            else
                print "0 0 "
            end
        end
        puts
    end
end

