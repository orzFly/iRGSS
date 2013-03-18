#
#   irb/locale.rb - internationalization module
#   	$Release Version: 0.9.5$
#   	$Revision: 11708 $
#   	$Date: 2007-02-13 08:01:19 +0900 (Tue, 13 Feb 2007) $
#   	by Keiju ISHITSUKA(keiju@ruby-lang.org)
#
# --
#
#   
#

module IRB
  class Locale
    @RCS_ID='-$Id: locale.rb 11708 2007-02-12 23:01:19Z shyouhei $-'


    def initialize(locale = nil)
      @lang = locale || ENV["IRB_LANG"] || ENV["LC_MESSAGES"] || ENV["LC_ALL"] || ENV["LANG"] || "C" 
    end

    attr_reader :lang

    def String(mes)
      mes
    end
  end
end