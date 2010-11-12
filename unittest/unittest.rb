#!/usr/bin/env ruby

require File.join(File.dirname(File.dirname(File.expand_path(__FILE__))),
                  'external/dist/share/service_testing/bp_service_runner.rb')
require 'uri'
require 'test/unit'
require 'open-uri'
require 'rbconfig'
include Config

class TestPubSub < Test::Unit::TestCase
  def setup
    subdir = 'build/PubSub'
    if ENV.key?('BP_OUTPUT_DIR')
      subdir = ENV['BP_OUTPUT_DIR']
    end
    @cwd = File.dirname(File.expand_path(__FILE__))
    @service = File.join(@cwd, "../#{subdir}")
    @providerDir = File.expand_path(File.join(@cwd, "providerDir"))
  end
  
  def teardown
  end

  def test_load_service
    BrowserPlus.run(@service, @providerDir) { |s|
    }
  end
end
