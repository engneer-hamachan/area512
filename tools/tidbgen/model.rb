# frozen_string_literal: true

module TiDatabaseGenerator
  BUILTIN_CLASS_FIELD_DEFINITIONS = {
    name_offset: "uint16_t",
    instance_method_start_index: "uint16_t",
    instance_method_count: "uint16_t",
    static_method_start_index: "uint16_t",
    static_method_count: "uint16_t"
  }.freeze

  BUILTIN_METHOD_FIELD_DEFINITIONS = {
    name_offset: "uint16_t",
    signature_offset: "uint16_t",
    document_offset: "uint16_t",
    return_class_identifier: "uint8_t",
    return_array_variant_class_identifier: "uint8_t",
    return_union_index: "uint16_t",
    array_variant_union_index: "uint16_t",
    block_parameter_class_identifier: "uint8_t",
    origin_class_identifier: "uint8_t"
  }.freeze

  CollectedClass = Struct.new(
    :full_name, :declaration_kind, :declarations,
    :direct_ancestors, :instance_methods, :static_methods,
    keyword_init: true
  )

  CollectedMethod = Struct.new(
    :name, :method_types, :comment, :origin_class_full_name,
    keyword_init: true
  )

  ResolvedCollectedMethod = Struct.new(
    :collected_method, :resolved_return_type,
    :block_parameter_class_identifier,
    keyword_init: true
  )

  CollectedDeclarations = Struct.new(
    :collected_classes, :type_aliases,
    keyword_init: true
  )

  ResolvedType = Struct.new(
    :class_identifiers, :array_variant_class_identifiers,
    keyword_init: true
  )

  BuiltinClassRecord = Struct.new(
    *BUILTIN_CLASS_FIELD_DEFINITIONS.keys, keyword_init: true
  )

  BuiltinMethodRecord = Struct.new(
    *BUILTIN_METHOD_FIELD_DEFINITIONS.keys, keyword_init: true
  )

  module ClassIdentifierCollection
    def self.normalize(class_identifiers:)
      class_identifiers.compact.reject(&:zero?).uniq.sort
    end
  end

end
