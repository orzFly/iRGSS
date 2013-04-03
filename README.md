iRGSS (Interactive RGSS)
========================

iRGSS is an interactive ruby which uses RGSS**????**.dll instead of ruby**\***.dll

Compiling
---------
### Requirements

* Microsoft Windows SDK 5.0+
* (cl) Microsoft 32-bit C/C++ Optimizing Compiler Version 15.00.30729.01 for 80x86
* (link) Microsoft Incremental Linker Version 9.00.30729.01
* (nmake) Microsoft Program Maintenance Utility Version 9.00.30729.01
* (ruby) ruby 1.8.7 (2011-06-30 patchlevel 352) [i386-mingw32]

### Building
```cmd
cmd> doskey make=nmake $*
cmd> git clone https://github.com/orzFly/iRGSS.git irgss
cmd> cd irgss
cmd> make
cmd> make test
```

Usage
-----
```shell
irgss [options] [-] [library names]
```
* `options` stand for any combination of the following options
* Only one library name is allowed without `-a`

Options
-------

### Core Options
#### -r `library`, --require `library`
Require `library`, before executing your script

#### -e `script`, --eval `script`
One line of script. Several `-e`'s allowed. Omit `-f`.

#### -f `script_fname`, --file `script_fname`
Load a file as your script. Only one allowed. Omit `-e`.

#### -d, --debug
Define $DEBUG.

### Mode Options
#### -i, --inspect
Need `-e` or `-f`. Scripts will be passed to IRB.

#### -b, --benchmark
Need `-e` or `-f`. Scripts will be wrapped in the following code: 
```ruby
require 'benchmark'
Benchmark.bm(10) do |benchmark|
  benchmark.report(IRGSS.IRBNAME) {
    # scripts go here
  }
end
```

#### -a, --batch
Take all libraries into consideration. So iRGSS will spawn itself for each library.

However, without `-a`, iRGSS will only take the first valid library.

### Screen Options

#### -s, --screen
Show RGSS screen at startup as you can use `IRGSS.screen.show` in your script later.

#### --screen-x `value-in-pixel`
#### --screen-y `value-in-pixel`
#### --screen-width `value-in-pixel`
#### --screen-height `value-in-pixel`
Specify the size and the location of the screen. You can use `IRGSS.screen` to play with your sweet RGSS screen in your script. Omit `--disable-window-hook`.

#### --screen-no-border

#### --screen-opacity `integer-from-0-to-255`

#### --allow-settings-dialog
Omit `--disable-screen-wndproc`

#### --allow-fullscreen
Omit `--disable-screen-wndproc`

#### --disable-background-running
Omit `--disable-screen-wndproc`

#### --hide-fps

### IRB Settings
#### -m, --irb-math
Bc mode (load mathn, fraction or matrix are available)

#### --irb-name `name`
Specify `IRGSS.IRBNAME`, which is passed to IRB. 

#### --irb-inspect
Use `inspect' for output (default except for bc mode)

#### --irb-noinspect
Don't use inspect for output

#### --irb-prompt `prompt-mode`, --irb-prompt-mode `prompt-mode`
Switch prompt mode. Pre-defined prompt modes are
* default
* simple
* xmp
* inf-ruby

#### --irb-inf-ruby-mode
Use prompt appropriate for inf-ruby-mode on emacs. 

#### --irb-simple-prompt
Simple prompt mode

#### --irb-noprompt
No prompt mode

#### --irb-tracer
Display trace for each execution of commands.

#### --irb-back-trace-limit `n`
Display backtrace top n and tail n. The default value is 16.

#### --irb-internal-debug `n`
Set internal debug level to n (not for popular use)

### RGSS Settings
#### --enable-bgm
#### --disable-bgm
Omit `--disable-registry-hook`. 

#### --enable-se
#### --disable-se
Omit `--disable-registry-hook`. 

#### --enable-vsync
#### --disable-vsync
Omit `--disable-registry-hook`. 

#### --keymap-default
Omit `--disable-registry-hook`. 
#### --keymap `keymap_string`
Omit `--disable-registry-hook`. 

#### --rtp
Omit `--disable-registry-hook`. 

#### --load-rtp `name=path`
Omit `--disable-registry-hook`. 

#### --title `title_string`

### Hooking Options
#### --disable-message-box-hook
#### --disable-window-hook
#### --disable-font-hook
#### --disable-registry-hook
#### --disable-screen-wndproc
#### --disable-audio-fallback

### Other Options
#### -v, --verbose
Turn on debug messages. 

#### -c `fname`, --rc `fname`
By default, IRGSS will parse `.irgssrc` for options before parsing the command line. IRGSS will find this file at following paths. If any of above hit, IRGSS will load it and stop finding.

* `./.irgssrc`
* `./irgssrc`
* `$HOME/.irgssrc`
* `$HOME/irgssrc`
* `$USERPROFILE/.irgssrc`
* `$USERPROFILE/irgssrc`

`.irgssrc` shares the same options with command lines. However, every options should be separated with `\r` or `\n`, but spaces.

#### -C, --no-rc
Suppress read of any `.irgssrc` and omit `-c`.

#### -v, --version
Print the version of IRGSS

#### -?, --help
Print this documentation.