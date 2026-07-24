# frozen_string_literal: true

module TiDatabaseGenerator
  class TypeResolver
    def initialize(type_aliases:, class_identifiers_by_full_name:)
      @type_aliases = type_aliases
      @class_identifiers_by_full_name = class_identifiers_by_full_name
    end

    def resolve(signature_type:, owner_full_name:)
      resolved_type_components =
        resolve_type_components(signature_type:, owner_full_name:)

      ResolvedType.new(
        class_identifiers:
          ClassIdentifierCollection.normalize(
            class_identifiers: resolved_type_components[:class_identifiers]
          ),
        array_variant_class_identifiers:
          ClassIdentifierCollection.normalize(
            class_identifiers: resolved_type_components[:array_variant_class_identifiers]
          )
      )
    end

    private

    def resolve_type_components(signature_type:, owner_full_name:)
      case signature_type
      when RBS::Types::ClassInstance
        resolve_class_instance_type_components(
          class_instance_type: signature_type,
          owner_full_name:
        )

      when RBS::Types::Union
        resolve_union_type_components(
          union_type: signature_type,
          owner_full_name:
        )

      when RBS::Types::Optional
        resolve_optional_type_components(
          optional_type: signature_type,
          owner_full_name:
        )

      when RBS::Types::Alias
        resolve_alias_type_components(
          alias_type: signature_type,
          owner_full_name:
        )

      when RBS::Types::Variable # T -> sub(T) -> T -> Untyped
        build_type_components(
          class_identifiers:
            [fetch_class_identifier(full_class_name: "::Untyped")]
        )

      when RBS::Types::Bases::Self, RBS::Types::Bases::Instance
        build_type_components(
          class_identifiers: [fetch_class_identifier(full_class_name: owner_full_name)]
        )

      when RBS::Types::Bases::Bool
        build_type_components(
          class_identifiers: [
            fetch_class_identifier(full_class_name: "::TrueClass"),
            fetch_class_identifier(full_class_name: "::FalseClass")
          ]
        )

      when RBS::Types::Bases::Nil, RBS::Types::Bases::Void
        build_type_components(
          class_identifiers: [fetch_class_identifier(full_class_name: "::NilClass")]
        )

      when RBS::Types::Bases::Any,
           RBS::Types::Bases::Top,
           RBS::Types::Bases::Bottom

        build_type_components(
          class_identifiers: [fetch_class_identifier(full_class_name: "::Untyped")]
        )

      when RBS::Types::Literal
        literal_full_class_name =
          resolve_literal_full_class_name(
            literal: signature_type.literal
          )

        build_type_components(
          class_identifiers: [fetch_class_identifier(full_class_name: literal_full_class_name)]
        )

      when RBS::Types::Proc
        build_type_components(
          class_identifiers: [fetch_class_identifier(full_class_name: "::Proc")]
        )

      when RBS::Types::Tuple,
           RBS::Types::Record,
           RBS::Types::Interface,
           RBS::Types::Intersection,
           RBS::Types::ClassSingleton

        build_type_components(
          class_identifiers: [fetch_class_identifier(full_class_name: "::Untyped")]
        )

      else
        raise "unsupported RBS type class: #{signature_type.class}"
      end
    end

    def resolve_class_instance_type_components(
      class_instance_type:,
      owner_full_name:
    )

      full_type_name =
        resolve_full_type_name(
          type_name: class_instance_type.name,
          owner_full_name:
        )

      # Array[T]
      if full_type_name == "::Array" && class_instance_type.args.first
        return resolve_array_type_components(
          element_type: class_instance_type.args.first,
          owner_full_name:
        )
      end

      # Hash[T] -> Hash[Untyped]
      if !class_instance_type.args.empty? && full_type_name != "::Array"
        return build_type_components(
          class_identifiers:
            [fetch_class_identifier(full_class_name: "::Untyped")]
        )
      end

      build_type_components(
        class_identifiers:
          [fetch_class_identifier(full_class_name: full_type_name)]
      )
    end

    def resolve_array_type_components(element_type:, owner_full_name:)
      element_type_components =
        resolve_type_components(signature_type: element_type, owner_full_name:)

      element_class_identifiers = element_type_components[:class_identifiers]

      unless element_type_components[:array_variant_class_identifiers].empty?
        element_class_identifiers =
          [fetch_class_identifier(full_class_name: "::Untyped")]
      end

      build_type_components(
        class_identifiers: [fetch_class_identifier(full_class_name: "::Array")],
        array_variant_class_identifiers: element_class_identifiers
      )
    end

    def resolve_union_type_components(union_type:, owner_full_name:)
      class_identifiers = []
      array_variant_class_identifiers = []

      union_type.types.each do |member_type|
        member_components =
          resolve_type_components(signature_type: member_type, owner_full_name:)

        class_identifiers.concat(member_components[:class_identifiers])

        array_variant_class_identifiers
          .concat(member_components[:array_variant_class_identifiers])
      end

      build_type_components(
        class_identifiers:,
        array_variant_class_identifiers:
      )
    end

    def resolve_optional_type_components(optional_type:, owner_full_name:)
      resolved_type_components =
        resolve_type_components(
          signature_type: optional_type.type,
          owner_full_name:
        )

      resolved_type_components[:class_identifiers] << fetch_class_identifier(full_class_name: "::NilClass")

      resolved_type_components
    end

    def resolve_alias_type_components(alias_type:, owner_full_name:)
      full_alias_name = resolve_full_type_name(type_name: alias_type.name, owner_full_name:)
      alias_declaration = @type_aliases[full_alias_name]

      # research toplevel
      unless alias_declaration
        top_level_full_alias_name = "::#{alias_type.name.name}"
        alias_declaration = @type_aliases[top_level_full_alias_name]
      end

      unless alias_declaration
        raise "#{owner_full_name}: unresolved type alias #{alias_type.name}"
      end

      alias_substitution =
        RBS::Substitution.build(
          alias_declaration.type_params.map(&:name), # [:A]
          alias_type.args # [String | Integer | Array | and more....]
        )

      resolve_type_components(
        signature_type: alias_declaration.type.sub(alias_substitution),
        owner_full_name:
      )
    end

    def resolve_full_type_name(type_name:, owner_full_name:)
      type_name_text = type_name.to_s
      return type_name_text if type_name_text.start_with?("::")

      owner_name_segments = owner_full_name.split("::")

      # search by ↓
      #::A::B::<type_name>
      #::A::<type_name>
      #::<type_name>
      while 1 < owner_name_segments.length
        full_type_name = "#{owner_name_segments.join("::")}::#{type_name_text}"

        if @class_identifiers_by_full_name.key?(full_type_name) ||
            @type_aliases.key?(full_type_name)

          return full_type_name
        end

        owner_name_segments.pop
      end

      "::#{type_name_text}"
    end

    def resolve_literal_full_class_name(literal:)
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

    def fetch_class_identifier(full_class_name:)
      unless @class_identifiers_by_full_name.key?(full_class_name)
        raise "class ID cannot be resolved: #{full_class_name}"
      end

      @class_identifiers_by_full_name[full_class_name]
    end

    def build_type_components(
      class_identifiers:,
      array_variant_class_identifiers: []
    )

      {
        class_identifiers:,
        array_variant_class_identifiers:
      }
    end

  end
end
