# frozen_string_literal: true

module TypeInformationDatabaseGenerator
  BuiltinDatabase = Struct.new(
    :generated_classes, :generated_methods, :class_identifiers,
    :enumeration_names, :name_pool, :signature_pool,
    :document_pool, :union_pool,
    keyword_init: true
  )

  class StringPool
    attr_reader :bytes

    def initialize(pool_name)
      @pool_name = pool_name
      @bytes = +"".b
      @string_offsets = {}
      add_string("")
    end

    def add_string(string)
      encoded_string = string.to_s.encode(Encoding::UTF_8)
      if @string_offsets.key?(encoded_string)
        return @string_offsets[encoded_string]
      end
      new_pool_size = @bytes.bytesize + encoded_string.bytesize + 1
      if new_pool_size > 65_535
        raise "#{@pool_name} string pool exceeds 65535 bytes (#{new_pool_size})"
      end

      @string_offsets[encoded_string] = @bytes.bytesize
      @bytes << encoded_string.b << "\0"
      @string_offsets[encoded_string]
    end
  end

  class UnionPool
    attr_reader :union_entries

    def initialize
      @union_entries = [[0, 0, 0, 0]]
      @union_indexes = { [] => 0 }
    end

    def add_union(class_identifiers, error_context)
      normalized_identifiers = ClassIdentifierCollection.normalize(
        class_identifiers
      )
      return 0 if normalized_identifiers.length <= 1
      if normalized_identifiers.length > 4
        raise "#{error_context}: union has #{normalized_identifiers.length} members"
      end
      if @union_indexes.key?(normalized_identifiers)
        return @union_indexes[normalized_identifiers]
      end
      if @union_entries.length >= 255
        raise "#{error_context}: union table exceeds 255 entries"
      end

      @union_indexes[normalized_identifiers] = @union_entries.length
      padding = Array.new(4 - normalized_identifiers.length, 0)
      @union_entries << (normalized_identifiers + padding)
      @union_indexes[normalized_identifiers]
    end
  end
end
