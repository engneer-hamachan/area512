class Area512::Repl
  PRIMARY_PROMPT = "irb> "
  CONTINUATION_PROMPT = "...> "

  def self.wait_for_sandbox(sandbox)
    signal_self_manage = Machine.pop_signal_self_manage

    while sandbox.state != :DORMANT && sandbox.state != :SUSPENDED
      Machine.check_signal unless signal_self_manage
      sleep_ms 5
    end

    true
  rescue Interrupt
    sandbox.stop
    write_line("^C")
    false
  end

  def self.write_sandbox_result(result)
    if result.is_a?(Exception)
      write_line("=> #{result.message} (#{result.class})")
    else
      write_line("=> #{result.inspect}")
    end
  end

  def self.start
    reset

    sandbox = Sandbox.new("irb")
    source = ""
    prompt = PRIMARY_PROMPT

    begin
      sandbox.compile("_ = nil")
      sandbox.area512_release_mrc_irep
      sandbox.execute
      wait_for_sandbox(sandbox)
      sandbox.suspend

      while true
        input_line = read_line(prompt)

        if input_line == :escape
          break

        elsif input_line == :out_of_memory
          write_line("Out of memory")
          break

        elsif input_line == :interrupt
          source = ""
          prompt = PRIMARY_PROMPT
          next

        elsif source.empty? && input_line.empty?
          next
        end

        source = source.empty? ? input_line : "#{source}\n#{input_line}"

        script = "begin; _ = (#{source}); rescue Exception => _; end; _"

        unless sandbox.compile(script, filename: "(irb)")
          prompt = CONTINUATION_PROMPT
          next
        end

        sandbox.area512_release_mrc_irep

        executed = sandbox.execute

        unless executed
          write_line("Execution failed")
          source = ""
          prompt = PRIMARY_PROMPT
          next
        end

        completed = wait_for_sandbox(sandbox)
        sandbox.suspend

        if completed
          result = sandbox.result
          write_sandbox_result(result)
        end

        source = ""
        prompt = PRIMARY_PROMPT
      end
    ensure
      sandbox.cleanup
    end

    nil
  end
end
