# Solitair -- automatic foundation moves and win check

class Solitair
  def elapsed_seconds
    now = Machine.uptime_us
    elapsed = (now - @start_us) / 1000000
    elapsed = 1 if elapsed < 1
    elapsed
  end

  def format_time(seconds)
    min = seconds / 60
    sec = seconds % 60
    sec_s = sec < 10 ? "0#{sec}" : "#{sec}"
    "#{min}:#{sec_s}"
  end

  def calc_score
    SCORE_TIME_BASE / @elapsed
  end

  def load_high_score
    @high_score = 0
    @high_elapsed = 0
    @high_moves = 0
    return unless SD.exist?(HIGHSCORE_FILE)
    line = SD.read(HIGHSCORE_FILE).split("\n")[0]
    if line.is_a?(String)
      parts = line.chomp.split(" ")
      @high_score = parts[0].to_i if parts.length > 0
      @high_elapsed = parts[1].to_i if parts.length > 1
      @high_moves = parts[2].to_i if parts.length > 2
    end
  rescue
    @high_score = 0
    @high_elapsed = 0
    @high_moves = 0
  end

  def save_high_score
    SD.write(HIGHSCORE_FILE, "#{@high_score} #{@high_elapsed} #{@high_moves}\n")
  rescue
    @msg = "Score #{@score}. Save failed."
  end

  def high_score_beaten?
    return true if @high_score <= 0
    return true if @score > @high_score
    return false if @score < @high_score
    return true if @high_elapsed <= 0
    return true if @elapsed < @high_elapsed
    return false if @elapsed > @high_elapsed
    @moves < @high_moves
  end

  def record_win
    return if @won_saved
    @elapsed = elapsed_seconds
    @score = calc_score
    if high_score_beaten?
      @high_score = @score
      @high_elapsed = @elapsed
      @high_moves = @moves
      save_high_score
      @msg = "New high #{@score} #{format_time(@elapsed)}. n new, q quit"
    else
      @msg = "Score #{@score} #{format_time(@elapsed)}. Best #{@high_score}. n new, q quit"
    end
    @won_saved = true
  end

  def auto_waste
    return false if @waste.empty?
    item = @waste[@waste.length - 1]
    if item.is_a?(Integer)
      card = item
      suit = card_suit(card)
      return false unless self.foundation_accept?(card, suit)
      @sel = 1
      @sel_idx = @waste.length - 1
      return self.move_to_foundation(suit)
    end
    false
  end

  def auto_tableau
    c = 0
    while c < 7
      pile = @tab[c]
      if !pile.empty?
        item = pile[pile.length - 1]
        if item.is_a?(Integer)
          card = item
          suit = card_suit(card)
          if face_up?(card) && self.foundation_accept?(card, suit)
            @sel = c + 6
            @sel_idx = pile.length - 1
            return self.move_to_foundation(suit)
          end
        end
      end
      c += 1
    end
    false
  end

  def auto_foundation
    moved = auto_waste
    moved = auto_tableau unless moved
    @msg = "No auto move." unless moved
  end

  def check_win
    @won = @found[0] == 13 && @found[1] == 13 && @found[2] == 13 && @found[3] == 13
    record_win if @won
  end
end
