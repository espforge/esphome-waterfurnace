#!/usr/bin/env ruby
# frozen_string_literal: true

# TCP-only ModBus mock server for testing
# Uses the waterfurnace_aurora gem's ModBus server extensions (func 65/66/67)
# Usage: ruby aurora_tcp_mock.rb <register_dump.yml> [port]

require "aurora"
require "logger"
require "yaml"

# The gem only includes Aurora::ModBus::Server into RTUServer.
# We need it for TCPServer too, to handle custom function codes 65/66/67.
ModBus::TCPServer.include(Aurora::ModBus::Server)

# Fix gem bug: process_func for func 67 uses `param` (singular) instead of
# `params` (plural), and only writes one register. Prepend a fix.
module Func67Fix
  def process_func(func, slave, req, params)
    if func == 67
      params.each { |param| slave.holding_registers[param[:addr]] = param[:val] }
      return func.chr + req[0, 2]
    end
    super
  end
end
ModBus::TCPServer.prepend(Func67Fix)

yaml_file = ARGV[0] || "fixtures/sample_registers.yml"
port = (ARGV[1] || 502).to_i

puts "Loading registers from #{yaml_file}..."
register_data = YAML.safe_load(File.read(yaml_file))

server = ModBus::TCPServer.new(port, host: "0.0.0.0")
server.logger = Logger.new($stdout, :info)

# Set up slaves 1 and 255 (AID Tool uses 1, some tools use 255)
slave1 = server.with_slave(1)
slave255 = server.with_slave(255)

r = slave1.holding_registers = slave255.holding_registers = Array.new(65_536, 0)

# Populate registers from YAML
register_data.each { |(k, v)| r[k] = v }

puts "Loaded #{register_data.size} registers"
puts "ModBus TCP server listening on port #{port}"
puts "Press Ctrl+C to stop"

server.request_callback = lambda do |uid, func, req|
  case func
  when 67
    puts "WRITE func 67 to slave #{uid}:"
    registers = req.map { |p| [p[:addr], p[:val]] }.to_h
    puts Aurora.print_registers(registers)
  when 65, 66, 3
    # Read requests - no output needed
  else
    puts "Unknown func #{func}"
  end
end

server.response_callback = lambda do |uid, func, res, req|
  case func
  when 65
    next unless res.is_a?(Array) && req
    register_list = []
    req.each { |params| register_list.concat(Range.new(params[:addr], params[:addr] + params[:quant], true).to_a) }
    puts "READ func 65 from slave #{uid}: #{register_list.length} registers"
  when 66
    next unless res.is_a?(Array) && req
    puts "READ func 66 from slave #{uid}: #{req.length} registers"
  when 3
    next unless res.is_a?(Array) && req
    puts "READ func 3 from slave #{uid}: #{req[:quant]} registers starting at #{req[:addr]}"
  end
end

server.start
sleep
