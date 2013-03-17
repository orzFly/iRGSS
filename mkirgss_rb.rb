#!/usr/bin/env ruby
#open "scripts", "wb" do |io| io.write Marshal.dump [[0,"iRGSS Initialize", Zlib::Deflate.deflate("eval((open('love.txt', 'rb') do |i| i.read end), binding, 'love.txt')")]] end
require 'zlib'

s = Marshal.dump [
  [
    0,
    "iRGSS Initialize",
    Zlib::Deflate.deflate(open('irgss.rb', 'rb') do |i| i.read end)
  ]
]
m = s.unpack("H*")[0].split(//).each_slice(2).map{|i|"0x#{i.join}"}.join(", ")
code = <<EOF
const char irgss_scripts[#{s.length}] = { #{m} };
EOF

# open "scripts", "wb" do |io| io.write s end

open "irgss_rb.h", "wb" do |io|
  io.write code
end