# frozen_string_literal: true

module TiDatabaseGenerator
  class SignatureManifest
    attr_reader :signature_paths

    def initialize(manifest_path:)
      @manifest_path = File.expand_path(manifest_path)
      @signature_paths = []

      instance_eval(File.read(@manifest_path), @manifest_path, 1)
    end

    def add_signature_path(signature_path:)
      absolute_signature_path =
        File.expand_path(
          signature_path,
          File.dirname(@manifest_path)
        )

      unless File.file?(absolute_signature_path)
        raise "manifest: RBS path does not exist: #{absolute_signature_path}"
      end

      unless @signature_paths.include?(absolute_signature_path)
        @signature_paths << absolute_signature_path
      end
    end
  end
end
