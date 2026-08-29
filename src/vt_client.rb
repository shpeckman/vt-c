# src/vt_client.rb
require 'socket'
require 'json'
require 'thread'

class VTClient
  attr_reader :socket_path, :daemon_path, :events

  def initialize(daemon_path = './bin/vt-daemon', socket_path = '/tmp/vt.sock')
    @daemon_path = daemon_path
    @socket_path = socket_path
    @events = Queue.new
    @process = nil
    @socket = nil
    @read_thread = nil
  end

  def start_daemon
    File.delete(@socket_path) if File.exist?(@socket_path)
    @process = Process.spawn(@daemon_path, '-s', @socket_path)
    wait_for_socket
  end

  def connect
    @socket = UNIXSocket.new(@socket_path)
    @read_thread = Thread.new { read_loop }
  end

  def write(data)
    raise "Not connected to daemon" unless @socket
    @socket.write(data)
    @socket.flush
  end

  def health?
    with_temp_socket do |sock|
      sock.print("\x1B_VTD;health\x1B\\")
      sock.flush
      while (line = sock.gets)
        begin
          json = JSON.parse(line)
          return true if json["status"] == "ok"
        rescue JSON::ParserError
        end
      end
    end
    false
  rescue StandardError
    false
  end

  def shutdown
    with_temp_socket do |sock|
      sock.print("\x1B_VTD;shutdown\x1B\\")
      sock.flush
    end
    Process.wait(@process) if @process
  rescue StandardError
  end

  def stop
    @events.close unless @events.closed?
    @socket&.close
    @socket = nil
    @read_thread&.kill
  end

  private

  def read_loop
    return unless @socket
    while !@socket.closed? && (line = @socket.gets)
      begin
        @events.push(JSON.parse(line))
      rescue JSON::ParserError
      rescue ClosedQueueError
        break
      end
    end
  rescue IOError
  ensure
    @events.close unless @events.closed?
  end

  def with_temp_socket
    sock = UNIXSocket.new(@socket_path)
    begin
      yield sock
    ensure
      sock.close
    end
  end

  def wait_for_socket
    30.times do
      return if File.exist?(@socket_path)
      sleep 0.1
    end
    raise "Daemon failed to start or bind to socket at #{@socket_path}"
  end
end

