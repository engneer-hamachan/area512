# frozen_string_literal: true

module TypeInformationDatabaseGenerator
  class DeclarationCollector
    def initialize(signature_declarations)
      @signature_declarations = signature_declarations
      @collected_classes = {}
      @type_aliases = {}
    end

    def collect
      collect_declarations(@signature_declarations, "::")

      CollectedDeclarations.new(
        classes: @collected_classes,
        type_aliases: @type_aliases
      )
    end

    private

    def collect_declarations(signature_declarations, namespace)
      signature_declarations.each do |declaration|
        case declaration
        when RBS::AST::Declarations::TypeAlias
          collect_type_alias(declaration, namespace)
        when RBS::AST::Declarations::Class, RBS::AST::Declarations::Module
          collect_class_declaration(declaration, namespace)
        end
      end
    end

    def collect_type_alias(declaration, namespace)
      full_alias_name = build_full_name(declaration.name, namespace)
      @type_aliases[full_alias_name] = declaration
    end

    def collect_class_declaration(declaration, namespace)
      full_name =
        build_full_name(
          declaration.name,
          namespace
        )

      declaration_kind = :module

      if declaration.is_a?(RBS::AST::Declarations::Class)
        declaration_kind = :class
      end

      collected_class =
        find_or_create_collected_class(
          full_name,
          declaration_kind
        )

      collected_class.declarations << declaration
      collect_superclass(collected_class, declaration)
      collect_members(collected_class, declaration.members)

      nested_declarations =
        declaration.members.grep(
          RBS::AST::Declarations::Base
        )

      collect_declarations(nested_declarations, full_name)
    end

    def find_or_create_collected_class(full_name, declaration_kind)
      collected_class = @collected_classes[full_name]
      if collected_class && collected_class.declaration_kind != declaration_kind
        raise "#{full_name}: class/module declaration conflict"
      end
      return collected_class if collected_class

      collected_class = CollectedClass.new(
        full_name: full_name,
        declaration_kind: declaration_kind,
        declarations: [],
        direct_ancestors: [],
        instance_methods: {},
        static_methods: {}
      )
      @collected_classes[full_name] = collected_class
      collected_class
    end

    def collect_superclass(collected_class, declaration)
      return unless declaration.respond_to?(:super_class)
      return unless declaration.super_class

      superclass_name = build_full_name(
        declaration.super_class.name
      )
      superclass_names = collected_class.direct_ancestors.filter_map do |ancestor|
        ancestor[1] if ancestor.first == :super
      end
      if !superclass_names.empty? && !superclass_names.include?(superclass_name)
        raise "#{collected_class.full_name}: conflicting superclasses"
      end
      return if superclass_names.include?(superclass_name)

      collected_class.direct_ancestors << [
        :super,
        superclass_name,
        declaration.super_class.args
      ]
    end

    def collect_members(collected_class, members)
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
          collect_ancestor(collected_class, member)
        when RBS::AST::Members::MethodDefinition
          collect_method_if_public(collected_class, member, visibility)
        when RBS::AST::Members::Alias
          collect_method_alias(collected_class, member)
        end
      end
    end

    def collect_ancestor(collected_class, member)
      ancestor_kind = member.class.name.split("::").last.downcase.to_sym
      ancestor_name = build_full_name(member.name)
      collected_class.direct_ancestors << [
        ancestor_kind,
        ancestor_name,
        member.args
      ]
    end

    def collect_method_if_public(collected_class, member, visibility)
      return if visibility == :private || member.visibility == :private

      collect_method(collected_class, member)
    end

    def collect_method(collected_class, member)
      overloads = member.overloads.map(&:method_type)
      collected_method = CollectedMethod.new(
        name: member.name.to_s,
        overloads: overloads,
        comment: clean_comment(member.comment&.string.to_s),
        origin_class_full_name: collected_class.full_name
      )
      store_method_by_kind(collected_class, collected_method, member.kind)
      collect_constructor(collected_class, collected_method, member)
    end

    def store_method_by_kind(collected_class, collected_method, method_kind)
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

    def collect_constructor(collected_class, collected_method, member)
      return unless member.name == :initialize

      collected_class.static_methods["new"] = CollectedMethod.new(
        name: "new",
        overloads: collected_method.overloads,
        comment: collected_method.comment,
        origin_class_full_name: collected_class.full_name,
        synthetic_return_class_full_name: collected_class.full_name
      )
    end

    def collect_method_alias(collected_class, member)
      method_table = collected_class.instance_methods
      if member.kind == :singleton
        method_table = collected_class.static_methods
      end
      original_method = method_table[member.old_name.to_s]
      unless original_method
        raise "#{collected_class.full_name}: alias #{member.new_name} has no source #{member.old_name}"
      end

      aliased_method = original_method.dup
      aliased_method.name = member.new_name.to_s
      method_table[member.new_name.to_s] = aliased_method
    end

    def clean_comment(comment)
      comment.lines.reject { |line| line.match?(/<!--.*-->/) }.join.strip
    end

    def build_full_name(type_name, namespace = "::")
      type_name_text = type_name.to_s
      return type_name_text if type_name_text.start_with?("::")

      namespace_prefix = "::"
      namespace_prefix = "#{namespace}::" unless namespace == "::"
      "#{namespace_prefix}#{type_name_text}"
    end
  end
end
