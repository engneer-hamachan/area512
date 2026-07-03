AREA512_ROOT = File.dirname(File.expand_path(__FILE__))
MRUBY_ROOT = File.join(AREA512_ROOT, "R2P2-ESP32/components/picoruby-esp32/picoruby")
IDF_BUILD_DIR = File.join(AREA512_ROOT, "build")
IDF_SDKCONFIG = File.join(AREA512_ROOT, "sdkconfig")
MRUBY_BUILD_CONFIG = File.join(AREA512_ROOT, "components/area512/build_config/area512.rb")
$LOAD_PATH << File.join(MRUBY_ROOT, "lib")

def idf_py(command)
  sh "idf.py #{command}"
end

def idf_cmake_build_dir?
  File.file?(File.join(IDF_BUILD_DIR, "CMakeCache.txt"))
end

def remove_invalid_idf_build_dir
  return unless File.directory?(IDF_BUILD_DIR)
  return if idf_cmake_build_dir?

  rm_rf IDF_BUILD_DIR
end

def remove_idf_sdkconfig
  rm_f IDF_SDKCONFIG
end

# load build systems
require "mruby/core_ext"
require "mruby/build"
require "picoruby/build"

# load configuration file. Default to the Area512 config so the top-level
# Rakefile does not fall back to picoruby's build_config/default.rb (which pulls
# in mruby/networking gems that Area512 has removed). CMake passes the same
# MRUBY_CONFIG to the inner picoruby build.
ENV['MRUBY_CONFIG'] ||= MRUBY_BUILD_CONFIG
MRUBY_CONFIG = MRuby::Build.mruby_config_path
load MRUBY_CONFIG

desc "Default task that runs all main tasks"
task :default => :all

desc "Build, flash, and monitor the Cardputer firmware"
task :all => %w[build flash monitor]

desc "Install dependencies and build mruby (first time only)"
task :setup do
  Rake::Task[:deep_clean].invoke
  remove_idf_sdkconfig
  idf_py "set-target esp32s3"
  FileUtils.cd MRUBY_ROOT do
    sh "bundle install"
    sh "rake"
  end
end

desc "Build the Cardputer firmware"
task :build do
  idf_py "build"
end

desc "Flash the built firmware"
task :flash do
  sh "idf.py flash"
end

desc "Flash the committed firmware image (no rebuild needed)"
task :flash_firmware do
  fw = File.join(AREA512_ROOT, "firmware")
  image = File.join(fw, "Area512.bin")
  raise "missing firmware image: #{image}" unless File.file?(image)

  port = ENV["ESPPORT"] ? "-p #{ENV['ESPPORT']} " : ""
  sh "esptool.py #{port}-c esp32s3 -b 460800 write_flash " \
     "--flash_mode dio --flash_size 8MB --flash_freq 80m 0x0 #{image}"
end

desc "Merge freshly built binaries into firmware/Area512.bin"
task :save_firmware do
  sh "make save-firmware"
end

desc "Erase factory partition and flash firmware binary"
task :flash_factory do
  sh "esptool.py -b 460800 erase_region 0x10000 0x400000"
  sh "esptool.py -b 460800 write_flash 0x10000 build/Area512.bin"
end

desc "Erase storage partition and flash storage binary"
task :flash_storage do
  sh "esptool.py -b 460800 erase_region 0x410000 0x300000"
  sh "esptool.py -b 460800 write_flash 0x410000 build/storage.bin"
end

desc "Monitor ESP32 serial output"
task :monitor do
  sh "idf.py monitor"
end

desc "Clean build artifacts"
task :clean do
  remove_invalid_idf_build_dir
  idf_py "clean" if idf_cmake_build_dir?
  FileUtils.cd MRUBY_ROOT do
    sh "MRUBY_CONFIG=#{MRUBY_BUILD_CONFIG} rake clean"
  end
end

desc "Perform deep clean including ESP32 build repos"
task :deep_clean => %w[clean] do
  idf_py "fullclean" if idf_cmake_build_dir?
  rm_rf File.join(MRUBY_ROOT, "build/repos/esp32")
end
