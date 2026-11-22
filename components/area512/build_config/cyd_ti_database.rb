def use_cyd_ti_database(spec, database_directory)
  include_path_index =
    spec.cc.include_paths.index("#{spec.dir}/src/generated")

  spec.cc.include_paths[include_path_index] = database_directory

  spec.objs.reject! do |object|
    object.start_with?("#{spec.build_dir}/src/generated/")
  end

  Dir["#{database_directory}/*.c"].each do |source|
    object =
      "#{spec.build_dir}/cyd_ti_generated/" \
      "#{spec.objfile(File.basename(source, ".c"))}"

    file object => source do |task|
      spec.cc.run task.name, task.prerequisites[0]
    end

    spec.objs << object
  end
end
