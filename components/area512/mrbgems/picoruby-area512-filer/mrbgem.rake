MRuby::Gem::Specification.new('picoruby-area512-filer') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachan'
  spec.summary = 'Native (C) framed-console file browser for Area512 (Cardputer)'

  spec.add_dependency 'picoruby-io-console'
  spec.cc.include_paths << "#{spec.dir}/../../../area512_hal/include"
  spec.cc.include_paths << "#{spec.dir}/../.."

  # mruby's default source glob is non-recursive; pick up src/ subdirectories.
  display_directory =
    if ENV['AREA512_CYD'] == 'ON'
      "cyd"
    elsif ENV['AREA512_EXT_DISPLAY'] == 'ON'
      "captft"
    end

  if display_directory
    spec.cc.include_paths << "#{spec.dir}/#{display_directory}/src"
  end

  spec.cc.include_paths << "#{spec.dir}/src"
  exts = spec.compilers.flat_map { |c| c.source_exts } * ","
  sources = Dir["#{spec.dir}/src/**/*{#{exts}}"]

  if display_directory
    sources = sources.reject { |f|
      File.file?(f.sub("#{spec.dir}/src/", "#{spec.dir}/#{display_directory}/src/"))
    } + Dir["#{spec.dir}/#{display_directory}/src/**/*{#{exts}}"]
  end

  spec.objs = sources
    .map { |f| spec.objfile(f.relative_path_from(spec.dir).to_s.pathmap("#{spec.build_dir}/%X")) }
end
