# frozen_string_literal: true

require './spec/spec_helper'

RSpec.describe IPPPrint::Error do
  let(:msg) { 'Test' }
  let(:error_number) { 1 }

  it 'responds to error_number and sql_state' do
    error = IPPPrint::Error.new(msg, error_number)

    expect(error.message).to eq(msg)
    expect(error.error_number).to eq(error_number)
  end
end
