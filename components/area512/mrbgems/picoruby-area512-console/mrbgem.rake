MRuby::Gem::Specification.new('picoruby-area512-console') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachan'
  spec.summary = 'On-screen console for Area512 app stdout (puts) and run errors'

  spec.cc.include_paths << "#{spec.dir}/../../../area512_hal/include"
end
