MRuby::Gem::Specification.new('picoruby-area512-filer') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachan'
  spec.summary = 'Native (C) framed-console file browser for Area512 (Cardputer)'

  spec.add_dependency 'picoruby-io-console'
  spec.cc.include_paths << "#{spec.dir}/../../../area512_hal/include"

  # mruby's default source glob is non-recursive; pick up src/ subdirectories.
  spec.cc.include_paths << "#{spec.dir}/src"
  exts = spec.compilers.flat_map { |c| c.source_exts } * ","
  spec.objs = Dir["#{spec.dir}/src/**/*{#{exts}}"]
    .map { |f| spec.objfile(f.relative_path_from(spec.dir).to_s.pathmap("#{spec.build_dir}/%X")) }
end
