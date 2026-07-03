MRuby::Gem::Specification.new('picoruby-area512-sdfat') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachan'
  spec.summary = 'microSD FAT access for Area512 (Cardputer)'

  spec.cc.include_paths << "#{spec.dir}/../../../area512_hal/include"
end
