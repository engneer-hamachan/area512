# frozen_string_literal: true

module TypeInformationDatabaseGenerator
  CollectedClass = Struct.new(
    :full_name, :declaration_kind, :declarations,
    :direct_ancestors, :instance_methods, :static_methods, :class_identifier,
    keyword_init: true
  )

  CollectedMethod = Struct.new(
    :name, :overloads, :comment, :origin_class_full_name,
    :synthetic_return_class_full_name,
    keyword_init: true
  )

  CollectedDeclarations = Struct.new(
    :classes, :type_aliases,
    keyword_init: true
  )

  ResolvedType = Struct.new(
    :class_identifiers, :array_variant_class_identifiers,
    keyword_init: true
  )

  GeneratedClassRecord = Struct.new(
    :name_offset, :instance_method_start, :instance_method_count,
    :static_method_start, :static_method_count, keyword_init: true
  )

  GeneratedMethodRecord = Struct.new(
    :name_offset, :signature_offset, :document_offset, :return_class_identifier,
    :return_array_variant_class_identifier, :return_union_index,
    :array_variant_union_index, :block_parameter_class_identifier,
    :origin_class_identifier,
    keyword_init: true
  )

  module ClassIdentifierCollection
    def self.normalize(class_identifiers)
      class_identifiers.compact.reject(&:zero?).uniq.sort
    end
  end
end
