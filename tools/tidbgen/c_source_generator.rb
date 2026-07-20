# frozen_string_literal: true

require "fileutils"

module TypeInformationDatabaseGenerator
  class CSourceGenerator
    def initialize(database)
      @database = database
    end

    def generate_header
      enumeration_lines = @database.enumeration_names.keys.sort.map do |class_identifier|
        enumeration_name = @database.enumeration_names[class_identifier]
        enumeration_line = "  #{enumeration_name},"
        if class_identifier.zero?
          enumeration_line = "  #{enumeration_name} = 0,"
        end
        enumeration_line
      end
      <<~HEADER
        #ifndef AREA512_TI_BUILTIN_DB_H
        #define AREA512_TI_BUILTIN_DB_H

        #include <stdint.h>

        typedef enum {
        #{enumeration_lines.join("\n")}
          TI_CLASS_BUILTIN_COUNT,
          TI_CLASS_USER_BASE = TI_CLASS_BUILTIN_COUNT
        } TiClassId;

        typedef struct {
          uint16_t name_offset;
          uint16_t signature_offset;
          uint16_t document_offset;
          uint8_t return_class_id;
          uint8_t return_array_variant_class_id;
          uint8_t return_union_index;
          uint8_t array_variant_union_index;
          uint8_t block_parameter_class_id;
          uint8_t origin_class_id;
        } TiBuiltinMethod;

        typedef struct {
          uint16_t name_offset;
          uint16_t instance_method_start;
          uint16_t instance_method_count;
          uint16_t static_method_start;
          uint16_t static_method_count;
        } TiBuiltinClass;

        typedef struct {
          uint8_t member_class_ids[4];
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
      source_code = +"#include \"area512_ti_builtin_db.h\"\n\n"
      source_code << generate_byte_pool(
        "ti_builtin_name_pool",
        @database.name_pool.bytes
      )
      source_code << generate_byte_pool(
        "ti_builtin_signature_pool",
        @database.signature_pool.bytes
      )
      source_code << generate_byte_pool(
        "ti_builtin_document_pool",
        @database.document_pool.bytes
      )
      source_code << generate_class_table
      source_code << generate_method_table
      source_code << generate_union_table
      source_code << generate_size_constants
      source_code
    end

    def write_files(output_directory)
      FileUtils.mkdir_p(output_directory)
      write_file_if_changed(
        File.join(output_directory, "area512_ti_builtin_db.h"),
        generate_header
      )
      write_file_if_changed(
        File.join(output_directory, "area512_ti_builtin_db.c"),
        generate_source
      )
    end

    private

    def generate_byte_pool(pool_name, pool_data)
      source_code = +"const char #{pool_name}[] = {\n"
      pool_data.bytes.each_slice(16) do |byte_slice|
        formatted_bytes = byte_slice.map { |byte| format("0x%02x,", byte) }
        source_code << "  #{formatted_bytes.join(' ')}\n"
      end
      source_code << "};\n\n"
    end

    def generate_class_table
      source_code = +"const TiBuiltinClass ti_builtin_classes[] = {\n"
      @database.generated_classes.each do |generated_class|
        class_fields = [
          generated_class.name_offset,
          generated_class.instance_method_start,
          generated_class.instance_method_count,
          generated_class.static_method_start,
          generated_class.static_method_count
        ]
        source_code << "  {#{class_fields.join(', ')}},\n"
      end
      source_code << "};\n\n"
    end

    def generate_method_table
      source_code = +"const TiBuiltinMethod ti_builtin_methods[] = {\n"
      @database.generated_methods.each do |generated_method|
        source_code << "  {#{generated_method.to_h.values.join(', ')}},\n"
      end
      source_code << "};\n\n"
    end

    def generate_union_table
      source_code = +"const TiBuiltinUnion ti_builtin_unions[] = {\n"
      @database.union_pool.union_entries.each do |union_entry|
        source_code << "  {{#{union_entry.join(', ')}}},\n"
      end
      source_code << "};\n\n"
    end

    def generate_size_constants
      <<~CONSTANTS
        const uint16_t ti_builtin_class_count = #{@database.generated_classes.length};
        const uint16_t ti_builtin_method_count = #{@database.generated_methods.length};
        const uint16_t ti_builtin_union_count = #{@database.union_pool.union_entries.length};
        const uint16_t ti_builtin_name_pool_size = #{@database.name_pool.bytes.bytesize};
        const uint16_t ti_builtin_signature_pool_size = #{@database.signature_pool.bytes.bytesize};
        const uint16_t ti_builtin_document_pool_size = #{@database.document_pool.bytes.bytesize};
      CONSTANTS
    end

    def write_file_if_changed(output_path, content)
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
