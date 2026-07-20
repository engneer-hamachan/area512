# frozen_string_literal: true

require "minitest/autorun"
require "tmpdir"
require_relative "../main"

module TypeInformationDatabaseGeneratorTestHelper
  FIXTURES = File.expand_path("fixtures", __dir__)

  def environment_for(*names)
    TypeInformationDatabaseGenerator::SignatureEnvironment.new(
      names.map { |name| File.join(FIXTURES, name) }
    )
  end
end
