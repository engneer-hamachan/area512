# frozen_string_literal: true

module TypeInformationDatabaseGenerator
  class TypeResolver
    def initialize(type_aliases, class_identifiers)
      @type_aliases = type_aliases
      @class_identifiers = class_identifiers
    end

    def resolve(signature_type, owner_full_name:, type_bindings: {})
      class_identifiers, array_variant_class_identifiers =
        resolve_type_components(
          signature_type,
          owner_full_name,
          type_bindings
        )
      ResolvedType.new(
        class_identifiers: ClassIdentifierCollection.normalize(class_identifiers),
        array_variant_class_identifiers: ClassIdentifierCollection.normalize(
          array_variant_class_identifiers
        )
      )
    end

    private

    def resolve_type_components(signature_type, owner_full_name, type_bindings)
      case signature_type
      when RBS::Types::ClassInstance
        resolve_class_instance(signature_type, owner_full_name, type_bindings)
      when RBS::Types::Union
        resolve_union(signature_type, owner_full_name, type_bindings)
      when RBS::Types::Optional
        resolve_optional(signature_type, owner_full_name, type_bindings)
      when RBS::Types::Alias
        resolve_alias(signature_type, owner_full_name, type_bindings)
      when RBS::Types::Variable
        resolve_variable(signature_type, owner_full_name, type_bindings)
      when RBS::Types::Bases::Self, RBS::Types::Bases::Instance
        [[fetch_class_identifier(owner_full_name)], []]
      when RBS::Types::Bases::Bool
        [
          [
            fetch_class_identifier("::TrueClass"),
            fetch_class_identifier("::FalseClass")
          ],
          []
        ]
      when RBS::Types::Bases::Nil, RBS::Types::Bases::Void
        [[fetch_class_identifier("::NilClass")], []]
      when RBS::Types::Bases::Any,
           RBS::Types::Bases::Top,
           RBS::Types::Bases::Bottom
        [[fetch_class_identifier("::Untyped")], []]
      when RBS::Types::Literal
        literal_class_name = resolve_literal_class_name(signature_type.literal)
        [[fetch_class_identifier(literal_class_name)], []]
      when RBS::Types::Proc
        [[fetch_class_identifier("::Proc")], []]
      when RBS::Types::Tuple,
           RBS::Types::Record,
           RBS::Types::Interface,
           RBS::Types::Intersection,
           RBS::Types::ClassSingleton
        [[fetch_class_identifier("::Untyped")], []]
      else
        raise "unsupported RBS type class: #{signature_type.class}"
      end
    end

    def resolve_class_instance(signature_type, owner_full_name, type_bindings)
      full_type_name = resolve_type_name(signature_type.name, owner_full_name)
      if full_type_name == "::Array" && signature_type.args.first
        return resolve_array(
          signature_type.args.first,
          owner_full_name,
          type_bindings
        )
      end
      if !signature_type.args.empty? && full_type_name != "::Array"
        return [[fetch_class_identifier("::Untyped")], []]
      end

      [[fetch_class_identifier(full_type_name)], []]
    end

    def resolve_array(element_type, owner_full_name, type_bindings)
      element_class_identifiers, nested_variant_class_identifiers =
        resolve_type_components(element_type, owner_full_name, type_bindings)
      unless nested_variant_class_identifiers.empty?
        element_class_identifiers = [fetch_class_identifier("::Untyped")]
      end
      [
        [fetch_class_identifier("::Array")],
        element_class_identifiers
      ]
    end

    def resolve_union(signature_type, owner_full_name, type_bindings)
      resolved_members = signature_type.types.map do |member_type|
        member_class_identifiers, member_variant_class_identifiers =
          resolve_type_components(
            member_type,
            owner_full_name,
            type_bindings
          )
        [member_class_identifiers, member_variant_class_identifiers]
      end
      [
        resolved_members.flat_map(&:first),
        resolved_members.flat_map(&:last)
      ]
    end

    def resolve_optional(signature_type, owner_full_name, type_bindings)
      class_identifiers, array_variant_class_identifiers =
        resolve_type_components(
          signature_type.type,
          owner_full_name,
          type_bindings
        )
      class_identifiers << fetch_class_identifier("::NilClass")
      [class_identifiers, array_variant_class_identifiers]
    end

    def resolve_alias(signature_type, owner_full_name, type_bindings)
      full_alias_name = resolve_type_name(signature_type.name, owner_full_name)
      alias_declaration = @type_aliases[full_alias_name]
      unless alias_declaration
        top_level_alias_name = "::#{signature_type.name.name}"
        alias_declaration = @type_aliases[
          top_level_alias_name
        ]
      end
      unless alias_declaration
        raise "#{owner_full_name}: unresolved type alias #{signature_type.name}"
      end

      alias_type_bindings = type_bindings.dup
      alias_declaration.type_params.zip(signature_type.args).each do |parameter, argument|
        alias_type_bindings[parameter.name] = argument
      end
      resolve_type_components(
        alias_declaration.type,
        owner_full_name,
        alias_type_bindings
      )
    end

    def resolve_variable(signature_type, owner_full_name, type_bindings)
      bound_type = type_bindings[signature_type.name]
      unless bound_type
        return [[fetch_class_identifier("::Untyped")], []]
      end

      resolve_type_components(bound_type, owner_full_name, type_bindings)
    end

    def resolve_type_name(type_name, owner_full_name)
      type_name_text = type_name.to_s
      return type_name_text if type_name_text.start_with?("::")

      namespace = owner_full_name.split("::")[0...-1].join("::")
      namespace_type_name = "#{namespace}::#{type_name_text}"
      if @class_identifiers.key?(namespace_type_name) ||
          @type_aliases.key?(namespace_type_name)
        return namespace_type_name
      end

      "::#{type_name_text}"
    end

    def resolve_literal_class_name(literal)
      case literal
      when Integer
        "::Integer"
      when Float
        "::Float"
      when String
        "::String"
      when Symbol
        "::Symbol"
      when true
        "::TrueClass"
      when false
        "::FalseClass"
      when nil
        "::NilClass"
      else
        raise "unsupported RBS literal: #{literal.inspect}"
      end
    end

    def fetch_class_identifier(full_class_name)
      unless @class_identifiers.key?(full_class_name)
        raise "class ID cannot be resolved: #{full_class_name}"
      end

      @class_identifiers[full_class_name]
    end

  end
end
