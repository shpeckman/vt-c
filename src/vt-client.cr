# src/vt-client.cr
require "socket"
require "json"
require "process"

class VTClient
  getter socket_path : String
  getter daemon_path : String
  getter events      : Channel(JSON::Any)

  @process : Process?
  @socket  : UNIXSocket?

  def initialize(@daemon_path = "./bin/vt-daemon", @socket_path = "/tmp/vt.sock")
    @events = Channel(JSON::Any).new
  end

  def start_daemon
    File.delete?(@socket_path)
    @process = Process.new(@daemon_path, ["-s", @socket_path])
    wait_for_socket
  end

  def connect
    @socket = UNIXSocket.new(@socket_path)
    spawn read_loop
  end

  def write(data : String | Bytes)
    if sock = @socket
      sock.write(data.to_slice)
      sock.flush
    else
      raise "Not connected to daemon"
    end
  end

  def health? : Bool
    with_temp_socket do |sock|
      sock.print("\x1B_VTD;health\x1B\\")
      sock.flush

      while line = sock.gets
        begin
          json = JSON.parse(line)
          return true if json["status"]? == "ok"
        rescue
          # Ignore non-JSON or other errors and keep reading
        end
      end
    end
    false
  rescue
    false
  end

  def shutdown
    with_temp_socket do |sock|
      sock.print("\x1B_VTD;shutdown\x1B\\")
      sock.flush
    end
    @process.try(&.wait)
  rescue
  end

  def stop
    @events.close unless @events.closed?
    @socket.try(&.close)
    @socket = nil
  end

  private def read_loop
    sock = @socket
    return unless sock

    while !sock.closed? && (line = sock.gets)
      begin
        @events.send(JSON.parse(line))
      rescue ex : JSON::ParseException
      rescue ex : Channel::ClosedError
        break
      end
    end
  rescue ex : IO::Error
  ensure
    @events.close unless @events.closed?
  end

  private def with_temp_socket(&)
    sock = UNIXSocket.new(@socket_path)
    begin
      yield sock
    ensure
      sock.close
    end
  end

  private def wait_for_socket
    30.times do
      return if File.exists?(@socket_path)
      sleep 0.1.seconds
    end
    raise "Daemon failed to start or bind to socket at #{@socket_path}"
  end
end
