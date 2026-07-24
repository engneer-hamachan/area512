# frozen_string_literal: true

module TiDatabaseGenerator
  BuiltinDatabase = Struct.new(
    :builtin_classes, :builtin_methods, :class_identifiers_by_full_name,
    :enumeration_names, :name_pool, :signature_pool,
    :document_pool, :union_pool,
    keyword_init: true
  )

  class StringPool
    attr_reader :binary_string

    def initialize(pool_name:)
      @pool_name = pool_name
      @binary_string = +"".b
      @string_offsets = {}
      add_string_and_return_offset(string: "")
    end

    def add_string_and_return_offset(string:)
      encoded_string = string.to_s.encode(Encoding::UTF_8)

      if @string_offsets.key?(encoded_string)
        return @string_offsets[encoded_string]
      end

      new_pool_byte_size = @binary_string.bytesize + encoded_string.bytesize + 1

      if new_pool_byte_size > 65_535
        raise "#{@pool_name} string pool exceeds 65535 bytes (#{new_pool_byte_size})"
      end

      @string_offsets[encoded_string] = @binary_string.bytesize
      @binary_string << encoded_string.b << "\0"

      @string_offsets[encoded_string]
    end
  end

  class UnionPool
    attr_reader :union_entries

    def initialize
      @union_entries = [[0, 0, 0, 0]]
      @union_indexes_by_class_identifiers = { [] => 0 }
    end

    def resolve_union_index(class_identifiers:, error_context:)
      normalized_class_identifiers =
        ClassIdentifierCollection.normalize(
          class_identifiers:
        )

      return 0 if normalized_class_identifiers.length <= 1

      if @union_indexes_by_class_identifiers.key?(normalized_class_identifiers)
        return @union_indexes_by_class_identifiers[normalized_class_identifiers]
      end

      if @union_entries.length >= 65_535
        raise "#{error_context}: union table exceeds 65535 entries"
      end

      @union_indexes_by_class_identifiers[normalized_class_identifiers] =
        @union_entries.length

      union_entry_padding =
        Array.new(4 - normalized_class_identifiers.length, 0)

      @union_entries << (normalized_class_identifiers + union_entry_padding)
      @union_indexes_by_class_identifiers[normalized_class_identifiers]
    end
  end
end
