MRuby::Gem::Specification.new('picoruby-area512-touch') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachan'
  spec.summary = 'Touch panel events and the software keyboard for Area512 (CYD)'

  spec.cc.include_paths << "#{spec.dir}/../../../area512_hal/include"
end
