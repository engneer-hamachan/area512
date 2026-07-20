# frozen_string_literal: true

require "pathname"
require "rbs"

module TypeInformationDatabaseGenerator
  class SignatureManifest
    attr_reader :signature_paths

    def initialize(manifest_path)
      @manifest_path = File.expand_path(manifest_path)
      @signature_paths = []
      instance_eval(File.read(@manifest_path), @manifest_path, 1)
    end

    def signature_file(signature_path)
      absolute_path = File.expand_path(
        signature_path,
        File.dirname(@manifest_path)
      )
      unless File.file?(absolute_path)
        raise "manifest: RBS path does not exist: #{absolute_path}"
      end

      @signature_paths << absolute_path unless @signature_paths.include?(absolute_path)
    end
  end

  class SignatureEnvironment
    attr_reader :resolved_environment, :signature_declarations, :type_aliases

    def initialize(signature_paths)
      environment_loader = RBS::EnvironmentLoader.new(core_root: nil)
      signature_paths.each do |signature_path|
        environment_loader.add(path: Pathname(signature_path))
      end
      @resolved_environment = RBS::Environment.from_loader(
        environment_loader
      ).resolve_type_names
      @signature_declarations = parse_signature_declarations(signature_paths)
      @type_aliases = {}
      collect_type_aliases(@signature_declarations, "::")
    rescue RBS::BaseError => error
      raise "RBS environment: #{error.message}"
    end

    def build_full_name(type_name, namespace = "::")
      type_name_text = type_name.to_s
      return type_name_text if type_name_text.start_with?("::")

      namespace_prefix = "::"
      namespace_prefix = "#{namespace}::" unless namespace == "::"
      "#{namespace_prefix}#{type_name_text}"
    end

    private

    def parse_signature_declarations(signature_paths)
      signature_paths.flat_map do |signature_path|
        begin
          RBS::Parser.parse_signature(
            File.read(signature_path)
          ).last
        rescue RBS::ParsingError => error
          raise "#{signature_path}: #{error.message}"
        end
      end
    end

    def collect_type_aliases(signature_declarations, namespace)
      signature_declarations.each do |declaration|
        case declaration
        when RBS::AST::Declarations::TypeAlias
          full_alias_name = build_full_name(declaration.name, namespace)
          @type_aliases[full_alias_name] = declaration
        when RBS::AST::Declarations::Class, RBS::AST::Declarations::Module
          nested_namespace = build_full_name(declaration.name, namespace)
          nested_declarations = declaration.members.grep(
            RBS::AST::Declarations::Base
          )
          collect_type_aliases(nested_declarations, nested_namespace)
        end
      end
    end
  end
end
