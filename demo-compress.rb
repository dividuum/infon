require 'zlib'
File.open(ARGV[1], "wb") do |out|
    out.write("\x00\xFE")
    out.write(Zlib::Deflate.deflate(File.read(ARGV[0]), Zlib::BEST_COMPRESSION))
end
