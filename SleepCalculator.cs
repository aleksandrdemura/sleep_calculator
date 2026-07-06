// SleepCalculator.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

public record SleepEntry(
    int Id,
    string Date,
    string Bedtime,
    string Wakeup,
    int Duration,
    int Quality
);

public class SleepData
{
    public List<SleepEntry> Entries { get; set; } = new();
}

public class SleepCalculator
{
    private const int CYCLE_MINUTES = 90;
    private List<SleepEntry> entries = new();
    private int nextId = 1;

    public IReadOnlyList<SleepEntry> Entries => entries.AsReadOnly();

    private TimeOnly ParseTime(string timeStr)
    {
        if (!TimeOnly.TryParseExact(timeStr, "HH:mm", out var t))
            throw new ArgumentException("Неверный формат времени, используйте ЧЧ:ММ");
        return t;
    }

    private string AddMinutesToTime(string timeStr, int minutes)
    {
        var t = ParseTime(timeStr);
        t = t.AddMinutes(minutes);
        return t.ToString("HH:mm");
    }

    public string CalculateWakeup(string bedtime, int cycles = 5)
    {
        if (cycles < 1 || cycles > 10) throw new ArgumentException("Количество циклов должно быть от 1 до 10");
        return AddMinutesToTime(bedtime, cycles * CYCLE_MINUTES);
    }

    public string CalculateBedtime(string wakeup, int cycles = 5)
    {
        if (cycles < 1 || cycles > 10) throw new ArgumentException("Количество циклов должно быть от 1 до 10");
        var t = ParseTime(wakeup);
        t = t.AddMinutes(-cycles * CYCLE_MINUTES);
        return t.ToString("HH:mm");
    }

    public SleepEntry AddEntry(string bedtime, string wakeup, int quality)
    {
        if (quality < 1 || quality > 5) throw new ArgumentException("Оценка качества должна быть от 1 до 5");
        var t1 = ParseTime(bedtime);
        var t2 = ParseTime(wakeup);
        if (t2 <= t1) t2 = t2.AddHours(24);
        var duration = (int)(t2 - t1).TotalMinutes;
        var entry = new SleepEntry(
            nextId,
            DateTime.Now.ToString("yyyy-MM-dd"),
            bedtime,
            wakeup,
            duration,
            quality
        );
        entries.Add(entry);
        nextId++;
        return entry;
    }

    public SleepEntry? FindEntry(int id) => entries.FirstOrDefault(e => e.Id == id);

    public bool DeleteEntry(int id) => entries.RemoveAll(e => e.Id == id) > 0;

    public Dictionary<string, object> GetStats()
    {
        int total = entries.Count;
        if (total == 0) return new Dictionary<string, object> { ["total"] = 0, ["avg_duration"] = null, ["avg_quality"] = null };
        return new Dictionary<string, object>
        {
            ["total"] = total,
            ["avg_duration"] = entries.Average(e => e.Duration),
            ["avg_quality"] = entries.Average(e => e.Quality)
        };
    }

    public void SaveToFile(string filename)
    {
        var data = new SleepData { Entries = entries };
        var options = new JsonSerializerOptions { WriteIndented = true };
        string json = JsonSerializer.Serialize(data, options);
        File.WriteAllText(filename, json);
    }

    public void LoadFromFile(string filename)
    {
        if (!File.Exists(filename)) return;
        string json = File.ReadAllText(filename);
        var data = JsonSerializer.Deserialize<SleepData>(json);
        if (data != null)
        {
            entries = data.Entries;
            nextId = entries.Any() ? entries.Max(e => e.Id) + 1 : 1;
        }
    }
}

public static class Program
{
    private static string ReadString(string prompt)
    {
        Console.Write(prompt);
        return Console.ReadLine()?.Trim() ?? "";
    }

    private static int ReadInt(string prompt)
    {
        while (true)
        {
            Console.Write(prompt);
            if (int.TryParse(Console.ReadLine(), out int result))
                return result;
            Console.WriteLine("Введите число.");
        }
    }

    private static void PrintEntry(SleepEntry entry)
    {
        string stars = new string('⭐', entry.Quality) + new string('☆', 5 - entry.Quality);
        Console.WriteLine($"#{entry.Id} - {entry.Date} | Лёг: {entry.Bedtime} | Проснулся: {entry.Wakeup}");
        Console.WriteLine($"   Продолжительность: {entry.Duration} мин ({entry.Duration/60}ч {entry.Duration%60}м)");
        Console.WriteLine($"   Качество: {stars} ({entry.Quality}/5)");
    }

    public static void Main()
    {
        var calc = new SleepCalculator();
        try { calc.LoadFromFile("sleep_data.json"); }
        catch { Console.WriteLine("Не удалось загрузить данные."); }

        while (true)
        {
            Console.WriteLine("\n===== КАЛЬКУЛЯТОР СНА (C#) =====");
            Console.WriteLine("1. Рассчитать время пробуждения");
            Console.WriteLine("2. Рассчитать время засыпания");
            Console.WriteLine("3. Добавить запись о сне");
            Console.WriteLine("4. Показать все записи");
            Console.WriteLine("5. Показать статистику");
            Console.WriteLine("6. Удалить запись");
            Console.WriteLine("7. Сохранить в файл");
            Console.WriteLine("8. Загрузить из файла");
            Console.WriteLine("0. Выход");
            string choice = ReadString("Выберите действие: ");

            switch (choice)
            {
                case "0": return;
                case "1":
                    string bedtime = ReadString("Введите время засыпания (ЧЧ:ММ): ");
                    string cyclesInput = ReadString("Введите количество циклов (по умолчанию 5): ");
                    int cycles = string.IsNullOrEmpty(cyclesInput) ? 5 : int.Parse(cyclesInput);
                    try
                    {
                        string wakeup = calc.CalculateWakeup(bedtime, cycles);
                        Console.WriteLine($"Вы должны проснуться в: {wakeup}");
                    }
                    catch (Exception ex) { Console.WriteLine($"Ошибка: {ex.Message}"); }
                    break;
                case "2":
                    string wakeup2 = ReadString("Введите время пробуждения (ЧЧ:ММ): ");
                    string cyclesInput2 = ReadString("Введите количество циклов (по умолчанию 5): ");
                    int cycles2 = string.IsNullOrEmpty(cyclesInput2) ? 5 : int.Parse(cyclesInput2);
                    try
                    {
                        string bedtime2 = calc.CalculateBedtime(wakeup2, cycles2);
                        Console.WriteLine($"Вам следует лечь спать в: {bedtime2}");
                    }
                    catch (Exception ex) { Console.WriteLine($"Ошибка: {ex.Message}"); }
                    break;
                case "3":
                    string btime = ReadString("Время засыпания (ЧЧ:ММ): ");
                    string wtime = ReadString("Время пробуждения (ЧЧ:ММ): ");
                    int quality = ReadInt("Оценка качества (1-5): ");
                    try
                    {
                        var entry = calc.AddEntry(btime, wtime, quality);
                        Console.WriteLine($"Запись добавлена с ID {entry.Id}");
                    }
                    catch (Exception ex) { Console.WriteLine($"Ошибка: {ex.Message}"); }
                    break;
                case "4":
                    if (!calc.Entries.Any()) Console.WriteLine("Записей нет.");
                    else foreach (var e in calc.Entries) PrintEntry(e);
                    break;
                case "5":
                    var stats = calc.GetStats();
                    if ((int)stats["total"] == 0) Console.WriteLine("Нет записей для статистики.");
                    else
                    {
                        Console.WriteLine("\n=== СТАТИСТИКА ===");
                        Console.WriteLine($"Всего записей: {stats["total"]}");
                        Console.WriteLine($"Средняя продолжительность: {stats["avg_duration"]:F1} мин ({(double)stats["avg_duration"]/60:F0}ч {(double)stats["avg_duration"]%60:F0}м)");
                        Console.WriteLine($"Средняя оценка качества: {stats["avg_quality"]:F2}/5");
                    }
                    break;
                case "6":
                    int id = ReadInt("Введите ID записи для удаления: ");
                    if (calc.DeleteEntry(id)) Console.WriteLine("Запись удалена.");
                    else Console.WriteLine("Запись не найдена.");
                    break;
                case "7":
                    try { calc.SaveToFile("sleep_data.json"); Console.WriteLine("Сохранено."); }
                    catch (Exception ex) { Console.WriteLine($"Ошибка: {ex.Message}"); }
                    break;
                case "8":
                    try { calc.LoadFromFile("sleep_data.json"); Console.WriteLine("Загружено."); }
                    catch (Exception ex) { Console.WriteLine($"Ошибка: {ex.Message}"); }
                    break;
                default: Console.WriteLine("Неизвестная команда."); break;
            }
        }
    }
}
