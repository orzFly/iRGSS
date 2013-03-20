#!/usr/bin/env ruby
#open "scripts", "wb" do |io| io.write Marshal.dump [[0,"iRGSS Initialize", Zlib::Deflate.deflate("eval((open('love.txt', 'rb') do |i| i.read end), binding, 'love.txt')")]] end
require 'zlib'

libs = {}
Dir.chdir("irgss_lib")
Dir['**/*.rb'].each { |i|
  libs[i] = [(open(i, 'rb') do |io| io.read end)].pack("m*").gsub!("\n", "")
}
Dir.chdir("..")
lib = libs.keys.map{ |i|
  "::IRGSS::LIB[#{i.inspect}] = #{libs[i].inspect}"
}.join("\n")

irgss_header = <<'EOFEOF' << lib << "\nEOF"
module ::IRGSS; end
eval <<'EOF', binding, 'iRGSS Header'
::IRGSS::REQUIRED = {}
::IRGSS::LIB = {}
::IRGSS::VERBOSE = 0

class << ::IRGSS
  def verbose j
    puts "irgss: rgss: " << j if IRGSS::VERBOSE == 1
  end
  def exception_inspect e
    puts $!
    puts $!.backtrace
  end
end

class ::Object
  def load filename
    Kernel.load filename
  end
  
  def require filename
    Kernel.require filename
  end
end

class << ::Kernel
  alias irgss_require  require
  alias irgss_load     load

  def irgss_extract_eval(filename)
    eval(::IRGSS::LIB[filename].unpack('m')[0], ::IRGSS::TOP_BINDING, ":irgss_lib/#{filename}")
  end

  def load(filename)
    if ::IRGSS::LIB.include? filename
       irgss_extract_eval(filename) 
       true
    elsif ::IRGSS::LIB.include?(filename + ".rb")
       irgss_extract_eval(filename + ".rb") 
       true
    else
       irgss_load(filename)
    end
  end

  def require(filename)
    return false if ::IRGSS::REQUIRED.include? filename
    ::IRGSS::REQUIRED[filename] = true
    if ::IRGSS::LIB.include? filename
       irgss_extract_eval(filename)
       true
    elsif ::IRGSS::LIB.include?(filename + ".rb")
       irgss_extract_eval(filename+".rb")
       true
    else
       irgss_require(filename)
    end
  end
end

EOFEOF

code = ""

{
  :irgss_scripts => Marshal.dump([
    [
      0,
      "iRGSS Initialize",
      Zlib::Deflate.deflate(open('irgss.rb', 'rb') do |i| i.read end)
    ]
  ]),
  
  :irgss_header => irgss_header,
}.each{ |k, v|
  m = v.unpack("H*")[0].split(//).each_slice(2).map{|i|"0x#{i.join}"}.join(", ")
  code << <<EOF
const char #{k}[#{v.length}] = { #{m} };
EOF
}

open "irgss_rb.h", "wb" do |io|
  io.write code
end