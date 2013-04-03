if (::IRGSS::PLATFORM == "RGSS1" && !require('rgss1hangup')) || ::IRGSS::PLATFORM != "RGSS1"
  RUBY_DESCRIPTION = "ruby #{RUBY_VERSION rescue nil}#{RUBY_PATCHLEVEL_STR rescue nil} (#{RUBY_RELEASE_DATE rescue nil}#{RUBY_REVISION_STR rescue nil}) [#{RUBY_PLATFORM rescue nil}]" unless (RUBY_DESCRIPTION rescue nil)

  if ::IRGSS::PLATFORM == "RGSS1" || ::IRGSS::PLATFORM == "RGSS2"
    module Kernel
      def self.p *args; STDOUT.p *args; end
      def self.print *args; STDOUT.print *args; end
    end
    class Object
      def p *args; STDOUT.p *args; end
      def print *args; STDOUT.print *args; end
    end
  end

  STDOUT.print <<EOF
iRGSS @ #{::IRGSS::PLATFORM} @ #{RUBY_DESCRIPTION}
EOF

  require 'irb'
  class << IRB
    alias init_config_irgss init_config
    def init_config(*args)
      init_config_irgss(*args)
      @CONF[:IRB_NAME] = ::IRGSS::IRBNAME
      @CONF[:SINGLE_IRB] = true
    end
  end
  
  IRB.start
end