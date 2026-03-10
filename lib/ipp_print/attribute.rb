# frozen_string_literal: true

module IPPPrint
  Attribute = Struct.new(:name, :value) do
    private_class_method :new # This is can only be called natively in C land
  end
end
