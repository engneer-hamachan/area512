# frozen_string_literal: true

module TiDatabaseGenerator
  class SignatureRenderer
    def render_method_signature(collected_method:)
      collected_method.method_types.map do |method_type|
        "#{collected_method.name}: #{method_type}"
      end.join("\n")
    end
  end
end
