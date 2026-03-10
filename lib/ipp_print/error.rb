# frozen_string_literal: true

module IPPPrint
  class Error < StandardError
    attr_reader :error_number

    def initialize(msg, error_number = nil)
      @error_number = error_number

      super(msg)
    end
  end
end
