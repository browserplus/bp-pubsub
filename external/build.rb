#!/usr/bin/env ruby

# XXX when 10.4 support dropped, get rid of this logic
if ARGV.length == 1 && ARGV[0] == "osx10.4"
  puts '*** DOING OSX10.4 BUILD **'
  ENV['BP_OSX_TARGET'] = '10.4'
else
  ENV['BP_OSX_TARGET'] = ''
end

require "./bakery/ports/bakery"

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
