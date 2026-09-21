MRuby::Gem::Specification.new('picoruby-area512-filer') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachan'
  spec.summary = 'Native (C) framed-console file browser for Area512 (Cardputer)'

  spec.add_dependency 'picoruby-io-console'
  spec.cc.include_paths << "#{spec.dir}/../../../area512_hal/include"

  # mruby's default source glob is non-recursive; pick up src/ subdirectories.
  if ENV['AREA512_EXT_DISPLAY'] == 'ON'
    spec.cc.include_paths << "#{spec.dir}/captft/src"
  end
  spec.cc.include_paths << "#{spec.dir}/src"
  exts = spec.compilers.flat_map { |c| c.source_exts } * ","
  sources = Dir["#{spec.dir}/src/**/*{#{exts}}"]
  if ENV['AREA512_EXT_DISPLAY'] == 'ON'
    sources = sources.reject { |f|
      File.file?(f.sub("#{spec.dir}/src/", "#{spec.dir}/captft/src/"))
    } + Dir["#{spec.dir}/captft/src/**/*{#{exts}}"]
  end
  spec.objs = sources
    .map { |f| spec.objfile(f.relative_path_from(spec.dir).to_s.pathmap("#{spec.build_dir}/%X")) }
end
