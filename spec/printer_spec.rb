# frozen_string_literal: true

require './spec/spec_helper'

# Test only works with a real printer
RSpec.describe IPPPrint::Printer do
  let(:job_name) { 'some_job' }
  let(:user_name) { 'some_user' }
  let(:format) { 'application/pdf' }

  subject { described_class.new('127.0.0.1', 631, timeout: 1000, protocol: :ipp) }

  context '#job_info' do
    it 'should return result' do
      File.open(File.join(__dir__, 'data/Test.pdf')) do |file|
        print_job_attributes = subject.print(file, format, job_name: job_name, user_name: user_name)
        job_uri = print_job_attributes.find { |attr| attr.name == 'job-uri' }.value

        expect(subject.job_info(job_uri)).not_to eq(nil)
      end
    end
  end

  context '#info' do
    it 'should return result' do
      expect(subject.info).not_to eq(nil)
    end
  end

  context '#print' do
    it 'should print and return job result' do
      File.open(File.join(__dir__, 'data/Test.pdf')) do |file|
        expect(subject.print(file, format, job_name: job_name, user_name: user_name)).not_to eq(nil)
      end
    end
  end
end
