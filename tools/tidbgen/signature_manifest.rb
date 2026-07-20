# frozen_string_literal: true

module TypeInformationDatabaseGenerator
  class SignatureManifest
    attr_reader :signature_paths

    def initialize(manifest_path)
      @manifest_path = File.expand_path(manifest_path)
      @signature_paths = []

      instance_eval(File.read(@manifest_path), @manifest_path, 1)
    end

    def signature_file(signature_path)
      absolute_path =
        File.expand_path(
          signature_path,
          File.dirname(@manifest_path)
        )

      unless File.file?(absolute_path)
        raise "manifest: RBS path does not exist: #{absolute_path}"
      end

      @signature_paths << absolute_path unless @signature_paths.include?(absolute_path)
    end
  end
end
