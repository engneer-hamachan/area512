MRuby::Gem::Specification.new('picoruby-area512-edit') do |spec|
  spec.license = 'MIT'
  spec.author  = 'hamachan'
  spec.summary = 'Area512 (Cardputer) native C vim-like editor and Markdown viewer'

  # main_task.rb / filer keep using `require "vim"` and `Vim.new(path).start`.
  spec.require_name = 'vim'

  # Syntax highlighter uses prism.
  spec.add_dependency 'mruby-compiler2'
  spec.add_dependency 'picoruby-io-console'
  spec.cc.include_paths << "#{spec.dir}/../../../area512_hal/include"
  spec.cc.include_paths << "#{MRUBY_ROOT}/mrbgems/mruby-compiler2/lib/prism/include"

  # mruby's default source glob is non-recursive; pick up src/ subdirectories.
  spec.cc.include_paths << "#{spec.dir}/src"
  exts = spec.compilers.flat_map { |c| c.source_exts } * ","
  spec.objs = Dir["#{spec.dir}/src/**/*{#{exts}}"]
    .map { |f| spec.objfile(f.relative_path_from(spec.dir).to_s.pathmap("#{spec.build_dir}/%X")) }
end
