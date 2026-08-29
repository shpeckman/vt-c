# t/client_test.cr
require "../src/vt-client"

client = VTClient.new("./bin/vt-daemon", "/tmp/vt_test.sock")

client.start_daemon

puts "health: #{client.health?}"

client.connect

spawn do
  while event = client.events.receive?
    puts event.to_json
  end
end

client.write("test\x1B[38:2:255:0:0mcolor\x1B[0m")

sleep 0.5.seconds

client.shutdown
client.stop
