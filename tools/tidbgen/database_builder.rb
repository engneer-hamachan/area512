# frozen_string_literal: true

module TypeInformationDatabaseGenerator
  FIXED_CLASS_ENUMERATION_NAMES = {
    "::Object" => "TI_CLASS_OBJECT",
    "::Integer" => "TI_CLASS_INTEGER",
    "::Float" => "TI_CLASS_FLOAT",
    "::String" => "TI_CLASS_STRING",
    "::Symbol" => "TI_CLASS_SYMBOL",
    "::Array" => "TI_CLASS_ARRAY",
    "::Hash" => "TI_CLASS_HASH",
    "::Range" => "TI_CLASS_RANGE",
    "::Proc" => "TI_CLASS_PROC",
    "::TrueClass" => "TI_CLASS_TRUE",
    "::FalseClass" => "TI_CLASS_FALSE",
    "::NilClass" => "TI_CLASS_NIL",
    "::Untyped" => "TI_CLASS_UNTYPED",
    "::Class" => "TI_CLASS_CLASS",
    "::Kernel" => "TI_CLASS_KERNEL"
  }.freeze

  class DatabaseBuilder
    def build(collected_declarations)
      @collected_classes = collected_declarations.classes
      allocate_class_identifiers
      @type_resolver = TypeResolver.new(
        collected_declarations.type_aliases,
        @class_identifiers
      )
      @signature_renderer = SignatureRenderer.new
      @name_pool = StringPool.new("name")
      @signature_pool = StringPool.new("signature")
      @document_pool = StringPool.new("document")
      @union_pool = UnionPool.new
      generate_tables

      BuiltinDatabase.new(
        generated_classes: @generated_classes,
        generated_methods: @generated_methods,
        class_identifiers: @class_identifiers,
        enumeration_names: @enumeration_names,
        name_pool: @name_pool,
        signature_pool: @signature_pool,
        document_pool: @document_pool,
        union_pool: @union_pool
      )
    end

    private

    def allocate_class_identifiers
      fixed_class_names = FIXED_CLASS_ENUMERATION_NAMES.keys
      missing_class_names = fixed_class_names.reject do |fixed_class_name|
        @collected_classes.key?(fixed_class_name)
      end
      unless missing_class_names.empty?
        raise "required RBS declarations are missing: #{missing_class_names.join(', ')}"
      end

      remaining_class_names = @collected_classes.keys - fixed_class_names
      @ordered_class_names = fixed_class_names + remaining_class_names.sort
      if @ordered_class_names.length >= 255
        raise "class ID exceeds uint8_t: #{@ordered_class_names.length}"
      end

      @class_identifiers = {}
      @enumeration_names = { 0 => "TI_CLASS_NONE" }
      @ordered_class_names.each_with_index do |full_class_name, class_name_index|
        class_identifier = class_name_index + 1
        @class_identifiers[full_class_name] = class_identifier
        enumeration_name = FIXED_CLASS_ENUMERATION_NAMES[full_class_name]
        unless enumeration_name
          enumeration_name = build_enumeration_name(full_class_name)
        end
        @enumeration_names[class_identifier] = enumeration_name
        collected_class = @collected_classes[full_class_name]
        collected_class.class_identifier = class_identifier
      end
    end

    def build_enumeration_name(full_class_name)
      normalized_name = full_class_name.delete_prefix("::").gsub("::", "_")
      normalized_name = normalized_name.gsub(/([a-z])([A-Z])/, '\\1_\\2')
      normalized_name = normalized_name.upcase.gsub(/[^A-Z0-9_]/, "_")
      "TI_CLASS_#{normalized_name}"
    end

    def generate_tables
      @generated_classes = Array.new(@class_identifiers.length + 1) do
        build_empty_generated_class
      end
      @generated_methods = []

      @ordered_class_names.each do |full_class_name|
        class_identifier = @class_identifiers[full_class_name]
        collected_class = @collected_classes[full_class_name]
        generated_class = @generated_classes[class_identifier]
        generated_class.name_offset = @name_pool.add_string(
          full_class_name.delete_prefix("::")
        )
        append_class_methods(collected_class, false, generated_class)
        append_class_methods(collected_class, true, generated_class)
      end
      if @generated_methods.length > 65_535
        raise "method table exceeds 65535 entries (#{@generated_methods.length})"
      end
    end

    def build_empty_generated_class
      GeneratedClassRecord.new(
        name_offset: 0,
        instance_method_start: 0,
        instance_method_count: 0,
        static_method_start: 0,
        static_method_count: 0
      )
    end

    def append_class_methods(collected_class, use_static_methods, generated_class)
      flattened_methods = collect_flattened_methods(
        collected_class,
        use_static_methods
      )
      start_field = :instance_method_start
      count_field = :instance_method_count
      if use_static_methods
        start_field = :static_method_start
        count_field = :static_method_count
      end
      generated_class[start_field] = @generated_methods.length
      flattened_methods.each do |collected_method, type_bindings|
        @generated_methods << generate_method_record(
          collected_class,
          collected_method,
          type_bindings
        )
      end
      generated_class[count_field] = flattened_methods.length
    end

    def collect_flattened_methods(collected_class, use_static_methods)
      methods_by_name = {}
      visited_class_names = {}
      collect_ancestor_methods(
        collected_class,
        collected_class,
        use_static_methods,
        {},
        methods_by_name,
        visited_class_names
      )
      object_class = @collected_classes["::Object"]
      if collected_class.full_name != "::Object" && object_class
        collect_ancestor_methods(
          object_class,
          collected_class,
          use_static_methods,
          {},
          methods_by_name,
          visited_class_names
        )
      end

      methods_by_name.keys.sort.map { |method_name| methods_by_name[method_name] }
    end

    def collect_ancestor_methods(
      current_class,
      owner_class,
      use_static_methods,
      type_bindings,
      methods_by_name,
      visited_class_names
    )
      return if visited_class_names[current_class.full_name]
      if current_class.full_name == "::Kernel" && current_class != owner_class
        return
      end

      visited_class_names[current_class.full_name] = true
      method_table = current_class.instance_methods
      method_table = current_class.static_methods if use_static_methods
      method_table.each do |method_name, collected_method|
        unless methods_by_name.key?(method_name)
          methods_by_name[method_name] = [collected_method, type_bindings]
        end
      end

      current_class.direct_ancestors.each do |ancestor|
        ancestor_kind, ancestor_name, type_arguments =
          ancestor
        if should_collect_ancestor?(ancestor_kind, use_static_methods)
          ancestor_class = @collected_classes[ancestor_name]
          if ancestor_class
            ancestor_bindings = bind_ancestor_types(
              ancestor_class,
              type_arguments,
              type_bindings
            )
            collect_ancestor_methods(
              ancestor_class,
              owner_class,
              use_static_methods,
              ancestor_bindings,
              methods_by_name,
              visited_class_names
            )
          end
        end
      end
    end

    def should_collect_ancestor?(ancestor_kind, use_static_methods)
      return ancestor_kind != :include if use_static_methods

      ancestor_kind != :extend
    end

    def bind_ancestor_types(ancestor_class, type_arguments, type_bindings)
      ancestor_bindings = type_bindings.dup
      type_parameters = ancestor_class.declarations.first.type_params
      type_parameters.zip(type_arguments).each do |parameter, argument|
        ancestor_bindings[parameter.name] = argument
      end
      ancestor_bindings
    end

    def generate_method_record(owner_class, collected_method, type_bindings)
      resolved_return_type = resolve_method_return_type(
        owner_class,
        collected_method,
        type_bindings
      )
      error_context = "#{owner_class.full_name}##{collected_method.name}"
      block_parameter_class_identifier = resolve_block_parameter_identifier(
        collected_method,
        owner_class.full_name,
        type_bindings
      )
      GeneratedMethodRecord.new(
        name_offset: @name_pool.add_string(collected_method.name),
        signature_offset: @signature_pool.add_string(
          @signature_renderer.render_method_signature(collected_method)
        ),
        document_offset: @document_pool.add_string(collected_method.comment),
        return_class_identifier: resolved_return_type.class_identifiers.first || 0,
        return_array_variant_class_identifier:
          resolved_return_type.array_variant_class_identifiers.first || 0,
        return_union_index: @union_pool.add_union(
          resolved_return_type.class_identifiers,
          "#{error_context} return type"
        ),
        array_variant_union_index: @union_pool.add_union(
          resolved_return_type.array_variant_class_identifiers,
          "#{error_context} array variant"
        ),
        block_parameter_class_identifier: block_parameter_class_identifier,
        origin_class_identifier: @class_identifiers.fetch(
          collected_method.origin_class_full_name
        )
      )
    end

    def resolve_method_return_type(owner_class, collected_method, type_bindings)
      if collected_method.synthetic_return_class_full_name
        return ResolvedType.new(
          class_identifiers: [
            @class_identifiers.fetch(
              collected_method.synthetic_return_class_full_name
            )
          ],
          array_variant_class_identifiers: []
        )
      end

      resolve_overload_return_types(
        collected_method,
        owner_class.full_name,
        type_bindings
      )
    end

    def resolve_overload_return_types(
      collected_method,
      owner_full_name,
      type_bindings
    )
      resolved_types = collected_method.overloads.map do |overload|
        @type_resolver.resolve(
          overload.type.return_type,
          owner_full_name: owner_full_name,
          type_bindings: type_bindings
        )
      end
      ResolvedType.new(
        class_identifiers:
          resolved_types.flat_map(&:class_identifiers).uniq.sort,
        array_variant_class_identifiers:
          resolved_types.flat_map(&:array_variant_class_identifiers).uniq.sort
      )
    end

    def resolve_block_parameter_identifier(
      collected_method,
      owner_full_name,
      type_bindings
    )
      collected_method.overloads.each do |overload|
        method_block = overload.block
        if method_block
          block_parameter = method_block.type.required_positionals.first
          unless block_parameter
            block_parameter = method_block.type.optional_positionals.first
          end
          if block_parameter
            resolved_type = @type_resolver.resolve(
              block_parameter.type,
              owner_full_name: owner_full_name,
              type_bindings: type_bindings
            )
            return resolved_type.class_identifiers.first || 0
          end
        end
      end
      0
    end
  end
end
