# frozen_string_literal: true

require "fileutils"

module TiDatabaseGenerator
  class CSourceGenerator
    def initialize(builtin_database:)
      @builtin_database = builtin_database
    end

    def generate_header
      enumeration_lines =
        @builtin_database.enumeration_names.keys.sort.map do |class_identifier|
          enumeration_name =
            @builtin_database.enumeration_names[class_identifier]

          enumeration_line = "  #{enumeration_name},"

          if class_identifier.zero?
            enumeration_line = "  #{enumeration_name} = 0,"
          end

          enumeration_line
        end

      builtin_method_field_declarations =
        generate_structure_field_declarations(
          structure_field_definitions: BUILTIN_METHOD_FIELD_DEFINITIONS
        )

      builtin_class_field_declarations =
        generate_structure_field_declarations(
          structure_field_definitions: BUILTIN_CLASS_FIELD_DEFINITIONS
        )

      <<~HEADER
        #ifndef AREA512_TI_BUILTIN_DATABASE_H
        #define AREA512_TI_BUILTIN_DATABASE_H

        #include <stdint.h>

        typedef enum {
        #{enumeration_lines.join("\n")}
          TI_CLASS_BUILTIN_COUNT,
          TI_CLASS_USER_BASE = TI_CLASS_BUILTIN_COUNT
        } TiClassIdentifier;

        typedef struct {
        #{builtin_method_field_declarations}
        } TiBuiltinMethod;

        typedef struct {
        #{builtin_class_field_declarations}
        } TiBuiltinClass;

        typedef struct {
          uint8_t member_class_identifiers[4];
        } TiBuiltinUnion;

        extern const TiBuiltinClass ti_builtin_classes[];
        extern const TiBuiltinMethod ti_builtin_methods[];
        extern const TiBuiltinUnion ti_builtin_unions[];
        extern const char ti_builtin_name_pool[];
        extern const char ti_builtin_signature_pool[];
        extern const char ti_builtin_document_pool[];
        extern const uint16_t ti_builtin_class_count;
        extern const uint16_t ti_builtin_method_count;
        extern const uint16_t ti_builtin_union_count;
        extern const uint16_t ti_builtin_name_pool_size;
        extern const uint16_t ti_builtin_signature_pool_size;
        extern const uint16_t ti_builtin_document_pool_size;

        #endif
      HEADER
    end

    def generate_source
      source_code = +"#include \"area512_ti_builtin_database.h\"\n\n"

      source_code << generate_byte_pool_source(
        pool_variable_name: "ti_builtin_name_pool",
        pool_binary_string: @builtin_database.name_pool.binary_string
      )

      source_code << generate_byte_pool_source(
        pool_variable_name: "ti_builtin_signature_pool",
        pool_binary_string: @builtin_database.signature_pool.binary_string
      )

      source_code << generate_byte_pool_source(
        pool_variable_name: "ti_builtin_document_pool",
        pool_binary_string: @builtin_database.document_pool.binary_string
      )

      source_code << generate_record_table_source(
        c_type_name: "TiBuiltinClass",
        c_variable_name: "ti_builtin_classes",
        records: @builtin_database.builtin_classes
      )

      source_code << generate_record_table_source(
        c_type_name: "TiBuiltinMethod",
        c_variable_name: "ti_builtin_methods",
        records: @builtin_database.builtin_methods
      )

      source_code << generate_union_table_source
      source_code << generate_size_constants_source

      source_code
    end

    def create_output_directory_and_write_files(output_directory_path:)
      FileUtils.mkdir_p(output_directory_path)

      write_file_if_changed(
        output_path: File.join(output_directory_path, "area512_ti_builtin_database.h"),
        content: generate_header
      )

      write_file_if_changed(
        output_path: File.join(output_directory_path, "area512_ti_builtin_database.c"),
        content: generate_source
      )
    end

    private

    def generate_byte_pool_source(pool_variable_name:, pool_binary_string:)
      source_code = +"const char #{pool_variable_name}[] = {\n"

      pool_binary_string.bytes.each_slice(16) do |byte_slice|
        formatted_byte_literals =
          byte_slice.map do |byte|
            format("0x%02x,", byte)
          end

        source_code << "  #{formatted_byte_literals.join(' ')}\n"
      end

      source_code << "};\n\n"
    end

    def generate_union_table_source
      generate_table_source(
        c_type_name: "TiBuiltinUnion",
        c_variable_name: "ti_builtin_unions",
        entries: @builtin_database.union_pool.union_entries
      ) do |union_entry|
        "{{#{union_entry.join(', ')}}}"
      end
    end

    def generate_size_constants_source
      <<~CONSTANTS
        const uint16_t ti_builtin_class_count = #{@builtin_database.builtin_classes.length};
        const uint16_t ti_builtin_method_count = #{@builtin_database.builtin_methods.length};
        const uint16_t ti_builtin_union_count = #{@builtin_database.union_pool.union_entries.length};
        const uint16_t ti_builtin_name_pool_size = #{@builtin_database.name_pool.binary_string.bytesize};
        const uint16_t ti_builtin_signature_pool_size = #{@builtin_database.signature_pool.binary_string.bytesize};
        const uint16_t ti_builtin_document_pool_size = #{@builtin_database.document_pool.binary_string.bytesize};
      CONSTANTS
    end

    def generate_structure_field_declarations(structure_field_definitions:)
      structure_field_definitions.map do |field_name, c_type_name|
        "  #{c_type_name} #{field_name};"
      end.join("\n")
    end

    def generate_record_table_source(c_type_name:, c_variable_name:, records:)
      generate_table_source(
        c_type_name:,
        c_variable_name:,
        entries: records
      ) do |record|
        "{#{record.to_h.values.join(', ')}}"
      end
    end

    def generate_table_source(c_type_name:, c_variable_name:, entries:)
      source_code = +"const #{c_type_name} #{c_variable_name}[] = {\n"
      entries.each do |entry|
        source_code << "  #{yield(entry)},\n"
      end
      source_code << "};\n\n"
    end

    def write_file_if_changed(output_path:, content:)
      if File.file?(output_path) && File.binread(output_path) == content
        return
      end

      temporary_path = "#{output_path}.tmp.#{$$}"

      File.binwrite(temporary_path, content)
      File.rename(temporary_path, output_path)

    ensure
      if temporary_path && File.exist?(temporary_path)
        File.delete(temporary_path)
      end
    end
  end
end
