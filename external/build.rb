#!/usr/bin/env ruby

require "bakery/ports/bakery"

topDir = File.dirname(File.expand_path(__FILE__));
$order = {
  :output_dir => File.join(topDir, "dist"),
  :packages => [
                "service_testing"
                ],
  :verbose => true,
}

b = Bakery.new $order
b.build
