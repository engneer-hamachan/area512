# frozen_string_literal: true

require_relative "test_helper"

class TypeInformationDatabaseGeneratorTest < Minitest::Test
  include TypeInformationDatabaseGeneratorTestHelper

  def test_manifest_deduplicates_paths_and_rejects_missing_paths
    Dir.mktmpdir do |directory|
      fixture = File.join(FIXTURES, "basic.rbs")
      manifest = File.join(directory, "manifest.rb")
      File.write(manifest, "signature_file #{fixture.inspect}\nsignature_file #{fixture.inspect}\n")
      assert_equal [fixture], TypeInformationDatabaseGenerator::SignatureManifest.new(manifest).signature_paths
      File.write(manifest, "signature_file #{File.join(directory, 'missing.rbs').inspect}\n")
      assert_raises(RuntimeError) { TypeInformationDatabaseGenerator::SignatureManifest.new(manifest) }
    end
  end

  def test_collector_method_kinds_initialize_alias_and_private
    entries = collected_declarations_for("method_kinds.rbs").classes
    entry = entries.fetch("::Kinds")
    assert entry.instance_methods.key?("both")
    assert entry.static_methods.key?("both")
    assert entry.static_methods.key?("new")
    assert entry.instance_methods.key?("copy")
    refute entry.instance_methods.key?("hidden")
  end

  def test_signature_keeps_optional_arguments_keywords_blocks_and_overloads
    entries = collected_declarations_for("arguments.rbs", "overloads.rbs").classes
    rendered = TypeInformationDatabaseGenerator::SignatureRenderer.new.render_method_signature(entries.fetch("::Arguments").instance_methods.fetch("call"))
    assert_includes rendered, "?String optional"
    assert_includes rendered, "key: Integer"
    assert_includes rendered, "?timeout: Integer"
    assert_includes rendered, "{ (String) -> void }"
    overload = TypeInformationDatabaseGenerator::SignatureRenderer.new.render_method_signature(entries.fetch("::Overloads").instance_methods.fetch("convert"))
    assert_equal 2, overload.lines.length
  end

  def test_return_type_resolves_optional_alias_array_self_literal_and_unsupported
    collected_declarations = collected_declarations_for(
      "basic.rbs", "aliases.rbs", "optional.rbs", "unsupported_types.rbs"
    )
    entries = collected_declarations.classes
    ids = entries.keys.sort.each_with_index.to_h { |name, index| [name, index + 1] }
    ids["::Untyped"] ||= ids.length + 1
    ids["::NilClass"] ||= ids.length + 1
    ids["::Array"] ||= ids.length + 1
    resolver = TypeInformationDatabaseGenerator::TypeResolver.new(
      collected_declarations.type_aliases,
      ids
    )
    optional = RBS::Parser.parse_type("String?")
    assert_equal [ids["::String"], ids["::NilClass"]].sort, resolver.resolve(optional, owner_full_name: "::Basic").class_identifiers
    array = resolver.resolve(RBS::Parser.parse_type("Array[String]"), owner_full_name: "::Basic")
    assert_equal [ids["::Array"]], array.class_identifiers
    assert_equal [ids["::String"]], array.array_variant_class_identifiers
    generic_alias = resolver.resolve(RBS::Parser.parse_type("maybe[String]"), owner_full_name: "::Aliases")
    assert_equal [ids["::String"], ids["::NilClass"]].sort, generic_alias.class_identifiers
    variable_optional = RBS::Parser.parse_method_type("[T] () -> T?").type.return_type
    bound_optional = resolver.resolve(
      variable_optional, owner_full_name: "::Optional",
      type_bindings: { T: RBS::Parser.parse_type("String") }
    )
    assert_equal [ids["::String"], ids["::NilClass"]].sort, bound_optional.class_identifiers
    assert_equal [ids["::Untyped"]], resolver.resolve(RBS::Parser.parse_type("[String, Integer]"), owner_full_name: "::Basic").class_identifiers
  end

  def test_database_and_c_source_are_deterministic
    collected_declarations = collected_declarations_for("basic.rbs", "inheritance.rbs")
    first = TypeInformationDatabaseGenerator::CSourceGenerator.new(
      TypeInformationDatabaseGenerator::DatabaseBuilder.new.build(
        collected_declarations
      )
    )
    reversed_declarations = collected_declarations.dup
    reversed_declarations.classes = collected_declarations.classes.to_a.reverse.to_h
    second = TypeInformationDatabaseGenerator::CSourceGenerator.new(
      TypeInformationDatabaseGenerator::DatabaseBuilder.new.build(
        reversed_declarations
      )
    )
    assert_equal first.generate_header, second.generate_header
    assert_equal first.generate_source, second.generate_source
    assert_includes first.generate_header, "uint8_t origin_class_id;"
  end

  def test_inherited_class_type_variable_is_bound_before_database_resolution
    collected_declarations = collected_declarations_for(
      "basic.rbs", "type_variables.rbs"
    )
    database = TypeInformationDatabaseGenerator::DatabaseBuilder.new.build(
      collected_declarations
    )
    class_identifier = database.class_identifiers.fetch("::StringBox")
    generated_class = database.generated_classes.fetch(class_identifier)
    method = database.generated_methods[generated_class.instance_method_start, generated_class.instance_method_count].find do |entry|
      database.name_pool.bytes.byteslice(entry.name_offset..).split("\0", 2).first == "value"
    end
    assert_equal database.class_identifiers.fetch("::String"), method.return_class_identifier
  end

  def test_union_and_string_pool_limits
    pool = TypeInformationDatabaseGenerator::UnionPool.new
    assert_raises(RuntimeError) { pool.add_union([1, 2, 3, 4, 5], "Test#method") }
    strings = TypeInformationDatabaseGenerator::StringPool.new("test")
    assert_raises(RuntimeError) { strings.add_string("x" * 65_535) }
  end

  def test_project_generator_and_checked_in_files_are_current
    manifest = File.expand_path("../manifest.rb", __dir__)
    generated = File.expand_path("../../../components/area512/mrbgems/picoruby-area512-ti/src/generated", __dir__)
    Dir.mktmpdir do |directory|
      TypeInformationDatabaseGenerator.generate_database_files(manifest, directory)
      %w[area512_ti_builtin_db.h area512_ti_builtin_db.c].each do |name|
        assert_equal File.binread(File.join(generated, name)), File.binread(File.join(directory, name)), "#{name} is stale"
      end
    end
  end
end
