puts "Success"
puts RUBY_VERSION
puts $RGSS_SCRIPTS[0][1].length
#(Kernel.method(:msgbox) rescue Kernel.method(:print)).call("兰兰大坏蛋")

a = Sprite.new
a.bitmap = Bitmap.new(640, 480)
a.bitmap.font.name = "Segoe UI"
a.bitmap.draw_text(0,0,640,480,"玖爷是个大坏蛋")
loop do Graphics.update end