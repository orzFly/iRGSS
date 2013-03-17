#(Kernel.method(:msgbox) rescue Kernel.method(:print)).call("兰兰大坏蛋")
puts "Success"
puts RUBY_VERSION
puts $RGSS_SCRIPTS[0][1].length