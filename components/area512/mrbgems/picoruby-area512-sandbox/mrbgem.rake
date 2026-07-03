MRuby::Gem::Specification.new('picoruby-area512-sandbox') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachan'
  spec.summary = 'Area512 extensions for PicoRuby Sandbox'

  spec.cc.include_paths << "#{spec.dir}/../../../area512_hal/include"

  spec.add_dependency 'picoruby-sandbox'
  spec.add_dependency 'picoruby-littlefs'
end
