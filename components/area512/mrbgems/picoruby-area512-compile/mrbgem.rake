MRuby::Gem::Specification.new('picoruby-area512-compile') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachan'
  spec.summary = 'On-device .rb -> .mrb compilation for Area512, without Sandbox'

  spec.cc.include_paths << "#{spec.dir}/../../../area512_hal/include"

  spec.add_dependency 'mruby-compiler2'
  # pm_options_t etc. live in prism's own headers, not the compiler2 includes.
  spec.cc.include_paths << "#{MRUBY_ROOT}/mrbgems/mruby-compiler2/lib/prism/include"
end
