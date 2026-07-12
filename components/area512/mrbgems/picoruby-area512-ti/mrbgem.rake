MRuby::Gem::Specification.new('picoruby-area512-ti') do |spec|
  spec.license = 'MIT'
  spec.author = 'hamachan'
  spec.summary = 'Area512 on-device completion engine'

  spec.add_dependency 'mruby-compiler2'
  spec.cc.include_paths << "#{spec.dir}/src"
  spec.cc.include_paths << "#{spec.dir}/src/core"
  spec.cc.include_paths << "#{spec.dir}/src/generated"
  spec.cc.include_paths << "#{MRUBY_ROOT}/mrbgems/mruby-compiler2/lib/prism/include"

  extensions = spec.compilers.flat_map { |compiler| compiler.source_exts } * ","
  spec.objs = Dir["#{spec.dir}/src/**/*{#{extensions}}"]
    .map do |source|
      relative_path = source.relative_path_from(spec.dir).to_s
      spec.objfile(relative_path.pathmap("#{spec.build_dir}/%X"))
    end
end
