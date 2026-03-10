# frozen_string_literal: true

require File.expand_path('lib/ipp_print/version', __dir__)

Gem::Specification.new do |gem|
  gem.name = 'ipp_print'
  gem.version = IPPPrint::VERSION
  gem.authors = ['Unact']
  gem.license = 'MIT'
  gem.email = 'it@unact.ru'
  gem.summary = 'Printing directly on a printer using ruby through IPP protocol'
  gem.extensions = ['ext/ipp_print/extconf.rb']
  gem.homepage = 'https://github.com/Unact/ipp_print'
  gem.files = `git ls-files README.md CHANGELOG.md LICENSE ext lib`.split

  gem.required_ruby_version = '>= 2.7.0'
  gem.metadata['rubygems_mfa_required'] = 'true'
end
