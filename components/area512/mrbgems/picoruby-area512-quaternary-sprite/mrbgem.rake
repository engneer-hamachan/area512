MRuby::Gem::Specification.new('picoruby-area512-quaternary-sprite') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachan'
  spec.summary = '4-bit palette sprite in the theme colors for Area512 apps'

  spec.cc.include_paths << "#{spec.dir}/../../../area512_hal/include"
end
