MRuby::Gem::Specification.new('picoruby-area512-widget') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachan'
  spec.summary = 'Native themed UI widgets for Area512 applications'

  spec.add_dependency 'picoruby-io-console'
  spec.cc.include_paths << "#{spec.dir}/../../../area512_hal/include"
  spec.cc.include_paths << "#{spec.dir}/src"

  # mruby's default source glob is not recursive.
  extensions = spec.compilers.flat_map { |compiler| compiler.source_exts } * ","
  spec.objs = Dir["#{spec.dir}/src/**/*{#{extensions}}"].map do |source|
    relative = source.relative_path_from(spec.dir).to_s
    spec.objfile(relative.pathmap("#{spec.build_dir}/%X"))
  end
end
