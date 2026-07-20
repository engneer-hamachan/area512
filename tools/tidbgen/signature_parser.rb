# frozen_string_literal: true

require "rbs"

module TypeInformationDatabaseGenerator
  class SignatureParser
    def parse(signature_paths)
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
  end
end
