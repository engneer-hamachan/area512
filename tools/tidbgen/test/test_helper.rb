# frozen_string_literal: true

require "minitest/autorun"
require "tmpdir"
require_relative "../main"

module TiDatabaseGeneratorTestHelper
  FIXTURE_DIRECTORY_PATH = File.expand_path("fixtures", __dir__)

  def build_collected_declarations_for(signature_file_names:)
    signature_paths =
      signature_file_names.map do |signature_file_name|
        File.join(FIXTURE_DIRECTORY_PATH, signature_file_name)
      end

    signature_declarations =
      TiDatabaseGenerator::SignatureParser.new.parse_signature_declarations(
        signature_paths:
      )

    TiDatabaseGenerator::DeclarationCollector.new(
      signature_declarations:
    ).build_collected_declarations
  end
end
