MRuby::Gem::Specification.new('picoruby-area512-repl') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachan'
  spec.summary = 'Ruby REPL for the Area512 on-screen console'

  spec.add_dependency 'picoruby-sandbox'
  spec.add_dependency 'picoruby-area512-sandbox'
  spec.add_dependency 'picoruby-io-console'
  spec.cc.include_paths << "#{spec.dir}/../../../area512_hal/include"
  spec.cc.include_paths << "#{spec.dir}/../../../area512_micropython/include"
end
