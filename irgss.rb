# encoding: utf-8
#==============================================================================
# ■ iRGSS Header
#------------------------------------------------------------------------------
# 　iRGSS 的 Ruby 部分的启动就靠这个啦。
#==============================================================================
# 对 RGSS1 启动紫苏的 Hangup 异常根除。
if (::IRGSS::PLATFORM == "RGSS1" && !require('rgss1hangup')) || ::IRGSS::PLATFORM != "RGSS1"
  # 对 Ruby 老版本构造 RUBY_DESCRIPTION 常量。
  unless (RUBY_DESCRIPTION rescue nil)
    RUBY_DESCRIPTION = "ruby #{RUBY_VERSION rescue nil}#{RUBY_PATCHLEVEL_STR rescue nil} (#{RUBY_RELEASE_DATE rescue nil}#{RUBY_REVISION_STR rescue nil}) [#{RUBY_PLATFORM rescue nil}]"
  end
  
  # RGSS1，RGSS2 恢复被重定向的 p, print
  if ::IRGSS::PLATFORM == "RGSS1" || ::IRGSS::PLATFORM == "RGSS2"
    class << Kernel
      alias msgbox_p  p
      alias msgbox    print
    end
    class Object
      def msgbox_p *args; Kernel.msgbox_p *args; end
      def msgbox *args; Kernel.msgbox *args; end
    end
    module Kernel
      def self.p *args; STDOUT.p *args; end
      def self.print *args; STDOUT.print *args; end
    end
    class Object
      def p *args; STDOUT.p *args; end
      def print *args; STDOUT.print *args; end
    end
  end
  
  # 处理 iRGSS 参数 -r, --require
  if (::IRGSS::VAR[:required])
    ::IRGSS.verbose "`-r` is present"
    ::IRGSS::VAR[:required].each do |i|
      ::IRGSS.verbose "requiring: #{i}"
      require i
    end
  end

  # 打印 Logo 除非 --no-logo
  unless (::IRGSS::VAR[:no_logo])
    STDOUT.puts "iRGSS @ #{::IRGSS::PLATFORM} @ #{RUBY_DESCRIPTION}"
  end

  require 'irb'
  class << IRB
    alias init_config_irgss init_config
    def init_config(*args)
      init_config_irgss(*args)
      (::IRGSS::VAR[:irb_conf]||{}).each do |k, v|
        @CONF[k] = v
      end
    end
  end
  
  IRB.start
end