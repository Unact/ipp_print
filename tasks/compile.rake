# frozen_string_literal: true

require 'rake/extensiontask'

Rake::ExtensionTask.new do |ext|
  ext.name = 'ipp_print'
  ext.lib_dir = 'lib/ipp_print'
  ext.ext_dir = 'ext/ipp_print'
  ext.tmp_dir = 'tmp'
  ext.source_pattern = '*.c'
  ext.gem_spec = Gem::Specification.load('ipp_print.gemspec')
end
