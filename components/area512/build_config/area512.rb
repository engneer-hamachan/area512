# Area512 build config: Cardputer (ESP32-S3, no PSRAM), femtoruby (mrubyc) VM only.
# Build name stays "esp32-femtoruby" so build/esp32-femtoruby paths in CMakeLists match.
MRuby::CrossBuild.new("esp32-femtoruby") do |conf|
  conf.toolchain("gcc")

  conf.cc.command = "xtensa-#{ENV['CONFIG_IDF_TARGET']}-elf-gcc"
  conf.linker.command = "xtensa-#{ENV['CONFIG_IDF_TARGET']}-elf-ld"
  conf.archiver.command = "xtensa-#{ENV['CONFIG_IDF_TARGET']}-elf-ar"

  conf.cc.host_command = "gcc"
  conf.cc.flags << "-Wall"
  conf.cc.flags << "-Wno-format"
  conf.cc.flags << "-Wno-unused-function"
  conf.cc.flags << "-Wno-maybe-uninitialized"
  conf.cc.flags << "-mlongcalls"

  conf.cc.defines << "MRBC_TICK_UNIT=10"
  conf.cc.defines << "MRBC_TIMESLICE_TICK_COUNT=1"
  conf.cc.defines << "MRBC_USE_FLOAT=2"
  conf.cc.defines << "MRBC_CONVERT_CRLF=1"
  conf.cc.defines << "ESP32_PLATFORM"
  conf.cc.defines << "PICORB_INT64"
  conf.cc.defines << "NDEBUG"
  conf.cc.defines << "MAX_SYMBOLS_COUNT=2000"
  conf.femtoruby(alloc_libc: false)

  # Pristine gems from ./R2P2-ESP32; the build root stays the Area512 tree.
  pure = File.expand_path('../../../R2P2-ESP32/components/picoruby-esp32/picoruby/mrbgems', __dir__)

  conf.gem gemdir: File.join(pure, 'mruby-compiler2')

  # machine/mrubyc/io-console must stay version-matched as a set; the esp32
  # machine port is kept in area512/ports/machine_port.c.
  conf.gem gemdir: File.join(pure, 'picoruby-machine')
  conf.gem gemdir: File.join(pure, 'picoruby-mrubyc')
  conf.gem gemdir: File.join(pure, 'picoruby-io-console')

  # littlefs core is pristine; the esp32 flash port lives in area512_hal.
  conf.gem gemdir: File.join(pure, 'picoruby-littlefs')

  # require first so Kernel#require is defined early.
  conf.gem gemdir: File.join(pure, 'picoruby-require')
  conf.gem gemdir: File.join(pure, 'picoruby-picorubyvm')
  conf.gem gemdir: File.join(pure, 'picoruby-time')
  conf.gem gemdir: File.join(pure, 'picoruby-vfs')
  conf.gem gemdir: File.join(pure, 'picoruby-watchdog')
  conf.gem gemdir: File.join(pure, 'picoruby-env')
  conf.gem gemdir: File.join(pure, 'picoruby-gpio')
  conf.gem gemdir: File.join(pure, 'picoruby-metaprog')

  # Runtime: edit Ruby files with vim and run them in an isolated mruby/c VM.
  conf.gem gemdir: File.join(pure, 'picoruby-sandbox')
  conf.gem gemdir: File.expand_path('../mrbgems/picoruby-area512-sandbox', __dir__)

  # Separate from Sandbox: the Sandbox compile path leaks heap per compile.
  conf.gem gemdir: File.expand_path('../mrbgems/picoruby-area512-compile', __dir__)

  # Native C editor replacing upstream vim/editor; still required as "vim".
  conf.gem gemdir: File.expand_path('../mrbgems/picoruby-area512-edit', __dir__)

  # Console/HAL C helpers: Cardputer keyboard (I2C) and battery ADC status.
  # Not exposed as user-app peripherals.
  conf.gem gemdir: File.join(pure, 'picoruby-rng')
  conf.gem gemdir: File.join(pure, 'picoruby-i2c')
  conf.gem gemdir: File.join(pure, 'picoruby-adc')

  # Flicker-free text drawing (Sprite) for the launcher and small apps.
  conf.gem gemdir: File.expand_path('../mrbgems/picoruby-area512-sprite', __dir__)

  # Native (C) framed-console file browser. Keeps the per-frame rendering and
  # input loop out of the mruby/c heap to avoid fragmentation as it grows.
  conf.gem gemdir: File.expand_path('../mrbgems/picoruby-area512-filer', __dir__)

  # microSD (FAT over SPI) VFS driver. Mounts at /sd; backend is ESP-IDF's
  # esp_vfs_fat_sdspi_mount (hardware init lives in area512_hal/area512_sd.c).
  conf.gem gemdir: File.expand_path('../mrbgems/picoruby-area512-sdfat', __dir__)
end
