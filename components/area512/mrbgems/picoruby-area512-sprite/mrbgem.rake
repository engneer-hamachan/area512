MRuby::Gem::Specification.new('picoruby-area512-sprite') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachan'
  spec.summary = 'LGFX sprite drawing for Area512 (Cardputer) apps'

  spec.cc.include_paths << "#{spec.dir}/../../../area512_hal/include"
end
