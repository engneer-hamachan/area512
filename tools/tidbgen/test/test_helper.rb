# frozen_string_literal: true

require "minitest/autorun"
require "tmpdir"
require_relative "../main"

module TypeInformationDatabaseGeneratorTestHelper
  FIXTURES = File.expand_path("fixtures", __dir__)

  def collected_declarations_for(*names)
    signature_paths = names.map { |name| File.join(FIXTURES, name) }
    signature_declarations =
      TypeInformationDatabaseGenerator::SignatureParser.new.parse(
        signature_paths
      )

    TypeInformationDatabaseGenerator::DeclarationCollector.new(
      signature_declarations
    ).collect
  end
end
