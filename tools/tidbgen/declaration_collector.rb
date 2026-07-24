# frozen_string_literal: true

module TiDatabaseGenerator
  class DeclarationCollector
    def initialize(signature_declarations:)
      @signature_declarations = signature_declarations
      @collected_classes = {}
      @type_aliases = {}
    end

    def build_collected_declarations
      collect_declarations(
        signature_declarations: @signature_declarations,
        containing_namespace: "::"
      )

      CollectedDeclarations.new(
        collected_classes: @collected_classes,
        type_aliases: @type_aliases
      )
    end

    private

    def collect_declarations(signature_declarations:, containing_namespace:)
      signature_declarations.each do |declaration|
        case declaration
        when RBS::AST::Declarations::TypeAlias
          collect_type_alias(
            type_alias_declaration: declaration,
            containing_namespace:
          )

        when RBS::AST::Declarations::Class, RBS::AST::Declarations::Module
          collect_class_declaration_and_nested_declarations(
            class_or_module_declaration: declaration,
            containing_namespace:
          )
        end
      end
    end

    def collect_type_alias(type_alias_declaration:, containing_namespace:)
      full_alias_name =
        build_full_type_name(
          type_name: type_alias_declaration.name,
          containing_namespace:
        )

      @type_aliases[full_alias_name] = type_alias_declaration
    end

    def collect_class_declaration_and_nested_declarations(
      class_or_module_declaration:,
      containing_namespace:
    )
      full_class_name =
        build_full_type_name(
          type_name: class_or_module_declaration.name,
          containing_namespace:
        )

      declaration_kind = :module

      if class_or_module_declaration.is_a?(RBS::AST::Declarations::Class)
        declaration_kind = :class
      end

      collected_class =
        find_or_create_collected_class(
          full_class_name:,
          declaration_kind:
        )

      collected_class.declarations << class_or_module_declaration

      collect_superclass(
        collected_class:,
        class_or_module_declaration:
      )

      collect_members(
        collected_class:,
        members: class_or_module_declaration.members
      )

      nested_declarations =
        class_or_module_declaration.members.grep(
          RBS::AST::Declarations::Base
        )

      collect_declarations(signature_declarations: nested_declarations, containing_namespace: full_class_name)
    end

    def find_or_create_collected_class(full_class_name:, declaration_kind:)
      collected_class = @collected_classes[full_class_name]

      if collected_class && collected_class.declaration_kind != declaration_kind
        raise "#{full_class_name}: class/module declaration conflict"
      end

      return collected_class if collected_class

      collected_class =
        CollectedClass.new(
          full_name: full_class_name,
          declaration_kind:,
          declarations: [],
          direct_ancestors: [],
          instance_methods: {},
          static_methods: {}
        )

      @collected_classes[full_class_name] = collected_class

      collected_class
    end

    def collect_superclass(collected_class:, class_or_module_declaration:)
      return unless class_or_module_declaration.respond_to?(:super_class)
      return unless class_or_module_declaration.super_class

      full_superclass_name =
        build_full_type_name(
          type_name: class_or_module_declaration.super_class.name
        )

      direct_superclass_full_names =
        collected_class.direct_ancestors.filter_map do |direct_ancestor|
          direct_ancestor[:full_name] if direct_ancestor[:kind] == :super
        end

      if !direct_superclass_full_names.empty? &&
          !direct_superclass_full_names.include?(full_superclass_name)

        raise "#{collected_class.full_name}: conflicting superclasses"
      end

      return if direct_superclass_full_names.include?(full_superclass_name)

      collected_class.direct_ancestors << {
        kind: :super,
        full_name: full_superclass_name,
        type_arguments: class_or_module_declaration.super_class.args
      }
    end

    def collect_members(collected_class:, members:)
      visibility = :public

      members.each do |member|
        case member
        when RBS::AST::Members::Public
          visibility = :public

        when RBS::AST::Members::Private
          visibility = :private

        when RBS::AST::Members::Include,
             RBS::AST::Members::Extend,
             RBS::AST::Members::Prepend

          collect_ancestor(collected_class:, ancestor_member: member)

        when RBS::AST::Members::MethodDefinition
          collect_method_if_public(collected_class:, method_member: member, visibility:)

        when RBS::AST::Members::Alias
          collect_method_alias(collected_class:, method_alias_member: member)
        end
      end
    end

    def collect_ancestor(collected_class:, ancestor_member:)
      ancestor_kind = ancestor_member.class.name.split("::").last.downcase.to_sym
      full_ancestor_name = build_full_type_name(type_name: ancestor_member.name)

      collected_class.direct_ancestors << {
        kind: ancestor_kind,
        full_name: full_ancestor_name,
        type_arguments: ancestor_member.args
      }
    end

    def collect_method_if_public(collected_class:, method_member:, visibility:)
      return if visibility == :private || method_member.visibility == :private

      collect_method_or_constructor(collected_class:, method_member:)
    end

    def collect_method_or_constructor(collected_class:, method_member:)
      method_types = method_member.overloads.map(&:method_type)

      collected_method =
        CollectedMethod.new(
          name: method_member.name.to_s,
          method_types:,
          comment:
            build_visible_comment(comment: method_member.comment&.string.to_s),
          origin_class_full_name: collected_class.full_name
        )

      store_method_by_kind(
        collected_class:,
        collected_method:,
        method_kind: method_member.kind
      ) unless method_member.name == :initialize

      collect_constructor(collected_class:, collected_method:, method_member:)
    end

    def store_method_by_kind(collected_class:, collected_method:, method_kind:)
      case method_kind
      when :instance
        collected_class.instance_methods[collected_method.name] = collected_method
      when :singleton
        collected_class.static_methods[collected_method.name] = collected_method
      when :singleton_instance
        collected_class.instance_methods[collected_method.name] = collected_method
        collected_class.static_methods[collected_method.name] = collected_method.dup
      end
    end

    def collect_constructor(collected_class:, collected_method:, method_member:)
      return unless method_member.name == :initialize

      constructor_return_type = RBS::Parser.parse_type(
        collected_class.full_name.delete_prefix("::")
      )
      constructor_method_types = collected_method.method_types.map do |method_type|
        method_type.update(
          type: method_type.type.with_return_type(constructor_return_type)
        )
      end

      collected_class.static_methods["new"] =
        CollectedMethod.new(
          name: "new",
          method_types: constructor_method_types,
          comment: collected_method.comment,
          origin_class_full_name: collected_class.full_name
        )
    end

    def collect_method_alias(collected_class:, method_alias_member:)
      method_table = collected_class.instance_methods

      if method_alias_member.kind == :singleton
        method_table = collected_class.static_methods
      end

      original_method =
        method_table[method_alias_member.old_name.to_s]

      unless original_method
        raise "#{collected_class.full_name}: alias #{method_alias_member.new_name} has no source #{method_alias_member.old_name}"
      end

      aliased_method = original_method.dup
      aliased_method.name = method_alias_member.new_name.to_s

      method_table[method_alias_member.new_name.to_s] = aliased_method
    end

    def build_visible_comment(comment:)
      comment.lines.reject { |line| line.match?(/<!--.*-->/) }.join.strip
    end

    def build_full_type_name(type_name:, containing_namespace: "::")
      type_name_text = type_name.to_s
      return type_name_text if type_name_text.start_with?("::")

      containing_namespace_prefix = "::"
      unless containing_namespace == "::"
        containing_namespace_prefix = "#{containing_namespace}::"
      end

      "#{containing_namespace_prefix}#{type_name_text}"
    end
  end
end
