#!/usr/bin/env ruby
# frozen_string_literal: true

require "optparse"

require_relative "model"
require_relative "signature_manifest"
require_relative "signature_parser"
require_relative "declaration_collector"
require_relative "signature_renderer"
require_relative "type_resolver"
require_relative "database"
require_relative "database_builder"
require_relative "c_source_generator"

module TiDatabaseGenerator
  def self.generate_database_files(
    manifest_path:,
    output_directory_path:
  )

    signature_manifest = SignatureManifest.new(manifest_path:)

    signature_declarations =
      SignatureParser.new.parse_signature_declarations(
        signature_paths: signature_manifest.signature_paths
      )

    collected_declarations =
      DeclarationCollector.new(
        signature_declarations:
      ).build_collected_declarations

    builtin_database =
      DatabaseBuilder.new.build(
        collected_declarations:
      )

    CSourceGenerator
      .new(builtin_database:)
      .create_output_directory_and_write_files(
        output_directory_path:
      )
  end

  def self.parse_command_line_arguments!(arguments:)
    argument_values = {}

    OptionParser.new do |parser|
      parser.on("--manifest PATH") do |manifest_path|
        argument_values[:manifest_path] = manifest_path
      end

      parser.on("--out DIRECTORY") do |output_directory_path|
        argument_values[:output_directory_path] = output_directory_path
      end
    end.parse!(arguments)

    argument_values
  end
end

if $PROGRAM_NAME == __FILE__
  begin
    argument_values =
      TiDatabaseGenerator.parse_command_line_arguments!(arguments: ARGV)

    unless argument_values[:manifest_path] &&
        argument_values[:output_directory_path]

      warn "usage: main.rb --manifest PATH --out DIR"

      exit 2
    end

    TiDatabaseGenerator.generate_database_files(
      manifest_path: argument_values[:manifest_path],
      output_directory_path: argument_values[:output_directory_path]
    )

  rescue StandardError => error
    warn error.message.lines.first.chomp

    exit 1
  end
end
