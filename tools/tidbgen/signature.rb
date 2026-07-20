# frozen_string_literal: true

module TypeInformationDatabaseGenerator
  class SignatureRenderer
    def render_method_signature(collected_method)
      collected_method.overloads.map do |method_type|
        rendered_type = method_type.to_s
        if collected_method.synthetic_return_class_full_name
          rendered_type = replace_return_type(
            rendered_type,
            collected_method.synthetic_return_class_full_name
          )
        end
        "#{collected_method.name}: #{rendered_type}"
      end.join("\n")
    end

    private

    def replace_return_type(rendered_type, synthetic_return_class_full_name)
      signature_without_return, arrow_separator, = rendered_type.rpartition(" -> ")
      display_return_type = synthetic_return_class_full_name.delete_prefix("::")
      "#{signature_without_return}#{arrow_separator}#{display_return_type}"
    end
  end
end
