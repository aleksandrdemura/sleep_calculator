# sleep_calculator.rb
require 'json'
require 'date'

CYCLE_MINUTES = 90

class SleepEntry
  attr_accessor :id, :date, :bedtime, :wakeup, :duration, :quality

  def initialize(id, date, bedtime, wakeup, duration, quality)
    @id = id
    @date = date
    @bedtime = bedtime
    @wakeup = wakeup
    @duration = duration
    @quality = quality
  end

  def to_h
    { id: @id, date: @date, bedtime: @bedtime, wakeup: @wakeup, duration: @duration, quality: @quality }
  end

  def self.from_h(hash)
    SleepEntry.new(hash[:id], hash[:date], hash[:bedtime], hash[:wakeup], hash[:duration], hash[:quality])
  end
end

class SleepCalculator
  attr_reader :entries

  def initialize
    @entries = []
    @next_id = 1
  end

  def parse_time(time_str)
    raise "Неверный формат времени" unless time_str =~ /^\d{2}:\d{2}$/
    h, m = time_str.split(':').map(&:to_i)
    raise "Неверные часы или минуты" unless (0..23).include?(h) && (0..59).include?(m)
    [h, m]
  end

  def add_minutes_to_time(time_str, minutes)
    h, m = parse_time(time_str)
    dt = DateTime.new(2000, 1, 1, h, m) + minutes / 1440.0
    dt.strftime("%H:%M")
  end

  def calculate_wakeup(bedtime, cycles = 5)
    raise "Количество циклов должно быть от 1 до 10" unless (1..10).include?(cycles)
    total = cycles * CYCLE_MINUTES
    add_minutes_to_time(bedtime, total)
  end

  def calculate_bedtime(wakeup, cycles = 5)
    raise "Количество циклов должно быть от 1 до 10" unless (1..10).include?(cycles)
    total = cycles * CYCLE_MINUTES
    h, m = parse_time(wakeup)
    dt = DateTime.new(2000, 1, 1, h, m) - total / 1440.0
    dt.strftime("%H:%M")
  end

  def add_entry(bedtime, wakeup, quality)
    raise "Оценка качества должна быть от 1 до 5" unless (1..5).include?(quality)
    h1, m1 = parse_time(bedtime)
    h2, m2 = parse_time(wakeup)
    dt1 = DateTime.new(2000, 1, 1, h1, m1)
    dt2 = DateTime.new(2000, 1, 1, h2, m2)
    dt2 += 1 if dt2 <= dt1
    duration = ((dt2 - dt1) * 24 * 60).to_i
    entry = SleepEntry.new(@next_id, Date.today.to_s, bedtime, wakeup, duration, quality)
    @entries << entry
    @next_id += 1
    entry
  end

  def find_entry(id)
    @entries.find { |e| e.id == id }
  end

  def delete_entry(id)
    entry = find_entry(id)
    return false unless entry
    @entries.delete(entry)
    true
  end

  def stats
    total = @entries.size
    return { total: 0, avg_duration: nil, avg_quality: nil } if total == 0
    durations = @entries.map(&:duration)
    qualities = @entries.map(&:quality)
    {
      total: total,
      avg_duration: durations.sum.to_f / total,
      avg_quality: qualities.sum.to_f / total
    }
  end

  def save_to_file(filename = "sleep_data.json")
    data = { entries: @entries.map(&:to_h) }
    File.write(filename, JSON.pretty_generate(data))
  end

  def load_from_file(filename = "sleep_data.json")
    return unless File.exist?(filename)
    data = JSON.parse(File.read(filename), symbolize_names: true)
    @entries.clear
    data[:entries].each do |item|
      entry = SleepEntry.from_h(item)
      @entries << entry
      @next_id = entry.id + 1 if entry.id >= @next_id
    end
  rescue JSON::ParserError
    puts "Ошибка чтения файла."
  end
end

def print_entry(entry)
  stars = "⭐" * entry.quality + "☆" * (5 - entry.quality)
  puts "##{entry.id} - #{entry.date} | Лёг: #{entry.bedtime} | Проснулся: #{entry.wakeup}"
  puts "   Продолжительность: #{entry.duration} мин (#{entry.duration/60}ч #{entry.duration%60}м)"
  puts "   Качество: #{stars} (#{entry.quality}/5)"
end

def main
  calc = SleepCalculator.new
  calc.load_from_file

  loop do
    puts "\n===== КАЛЬКУЛЯТОР СНА (Ruby) ====="
    puts "1. Рассчитать время пробуждения"
    puts "2. Рассчитать время засыпания"
    puts "3. Добавить запись о сне"
    puts "4. Показать все записи"
    puts "5. Показать статистику"
    puts "6. Удалить запись"
    puts "7. Сохранить в файл"
    puts "8. Загрузить из файла"
    puts "0. Выход"
    print "Выберите действие: "
    choice = gets.chomp

    case choice
    when "0"
      break
    when "1"
      print "Введите время засыпания (ЧЧ:ММ): "
      bedtime = gets.chomp
      print "Введите количество циклов (по умолчанию 5): "
      cycles_input = gets.chomp
      cycles = cycles_input.empty? ? 5 : cycles_input.to_i
      begin
        wakeup = calc.calculate_wakeup(bedtime, cycles)
        puts "Вы должны проснуться в: #{wakeup}"
      rescue => e
        puts "Ошибка: #{e.message}"
      end
    when "2"
      print "Введите время пробуждения (ЧЧ:ММ): "
      wakeup = gets.chomp
      print "Введите количество циклов (по умолчанию 5): "
      cycles_input = gets.chomp
      cycles = cycles_input.empty? ? 5 : cycles_input.to_i
      begin
        bedtime = calc.calculate_bedtime(wakeup, cycles)
        puts "Вам следует лечь спать в: #{bedtime}"
      rescue => e
        puts "Ошибка: #{e.message}"
      end
    when "3"
      print "Время засыпания (ЧЧ:ММ): "
      bedtime = gets.chomp
      print "Время пробуждения (ЧЧ:ММ): "
      wakeup = gets.chomp
      print "Оценка качества (1-5): "
      quality = gets.chomp.to_i
      begin
        entry = calc.add_entry(bedtime, wakeup, quality)
        puts "Запись добавлена с ID #{entry.id}"
      rescue => e
        puts "Ошибка: #{e.message}"
      end
    when "4"
      if calc.entries.empty?
        puts "Записей нет."
      else
        calc.entries.each { |e| print_entry(e) }
      end
    when "5"
      stats = calc.stats
      if stats[:total] == 0
        puts "Нет записей для статистики."
      else
        puts "\n=== СТАТИСТИКА ==="
        puts "Всего записей: #{stats[:total]}"
        puts "Средняя продолжительность: #{'%.1f' % stats[:avg_duration]} мин (#{stats[:avg_duration]/60}ч #{stats[:avg_duration]%60}м)"
        puts "Средняя оценка качества: #{'%.2f' % stats[:avg_quality]}/5"
      end
    when "6"
      print "Введите ID записи для удаления: "
      id = gets.chomp.to_i
      if calc.delete_entry(id)
        puts "Запись удалена."
      else
        puts "Запись не найдена."
      end
    when "7"
      calc.save_to_file
      puts "Сохранено."
    when "8"
      calc.load_from_file
      puts "Загружено."
    else
      puts "Неизвестная команда."
    end
  end
end

main if __FILE__ == $0
