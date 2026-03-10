# frozen_string_literal: true

require 'mkmf'

extension_name = 'ipp_print'

have_library('cups')

dir_config(extension_name)

create_makefile("#{extension_name}/#{extension_name}")
