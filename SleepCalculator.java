// SleepCalculator.java
import java.io.*;
import java.nio.file.*;
import java.time.*;
import java.time.format.*;
import java.util.*;
import java.util.stream.Collectors;

record SleepEntry(int id, String date, String bedtime, String wakeup, int duration, int quality) implements Serializable {}

class SleepData implements Serializable {
    private static final long serialVersionUID = 1L;
    public List<SleepEntry> entries;
}

class SleepCalculator implements Serializable {
    private static final long serialVersionUID = 1L;
    private static final int CYCLE_MINUTES = 90;
    private List<SleepEntry> entries = new ArrayList<>();
    private int nextId = 1;

    private LocalTime parseTime(String timeStr) {
        try {
            return LocalTime.parse(timeStr, DateTimeFormatter.ofPattern("HH:mm"));
        } catch (DateTimeParseException e) {
            throw new IllegalArgumentException("Неверный формат времени, используйте ЧЧ:ММ");
        }
    }

    private String addMinutesToTime(String timeStr, int minutes) {
        LocalTime t = parseTime(timeStr);
        t = t.plusMinutes(minutes);
        return t.format(DateTimeFormatter.ofPattern("HH:mm"));
    }

    public String calculateWakeup(String bedtime, int cycles) {
        if (cycles < 1 || cycles > 10) throw new IllegalArgumentException("Количество циклов должно быть от 1 до 10");
        int total = cycles * CYCLE_MINUTES;
        return addMinutesToTime(bedtime, total);
    }

    public String calculateBedtime(String wakeup, int cycles) {
        if (cycles < 1 || cycles > 10) throw new IllegalArgumentException("Количество циклов должно быть от 1 до 10");
        int total = cycles * CYCLE_MINUTES;
        LocalTime t = parseTime(wakeup);
        t = t.minusMinutes(total);
        return t.format(DateTimeFormatter.ofPattern("HH:mm"));
    }

    public SleepEntry addEntry(String bedtime, String wakeup, int quality) {
        if (quality < 1 || quality > 5) throw new IllegalArgumentException("Оценка качества должна быть от 1 до 5");
        LocalTime t1 = parseTime(bedtime);
        LocalTime t2 = parseTime(wakeup);
        if (t2.isBefore(t1) || t2.equals(t1)) {
            t2 = t2.plusHours(24);
        }
        long duration = Duration.between(t1, t2).toMinutes();
        SleepEntry entry = new SleepEntry(
            nextId,
            LocalDate.now().toString(),
            bedtime,
            wakeup,
            (int)duration,
            quality
        );
        entries.add(entry);
        nextId++;
        return entry;
    }

    public Optional<SleepEntry> findEntry(int id) {
        return entries.stream().filter(e -> e.id() == id).findFirst();
    }

    public boolean deleteEntry(int id) {
        return entries.removeIf(e -> e.id() == id);
    }

    public Map<String, Object> getStats() {
        int total = entries.size();
        if (total == 0) {
            return Map.of("total", 0, "avg_duration", null, "avg_quality", null);
        }
        double avgDur = entries.stream().mapToInt(SleepEntry::duration).average().orElse(0);
        double avgQual = entries.stream().mapToInt(SleepEntry::quality).average().orElse(0);
        return Map.of(
            "total", total,
            "avg_duration", avgDur,
            "avg_quality", avgQual
        );
    }

    public void saveToFile(String filename) throws IOException {
        SleepData data = new SleepData();
        data.entries = new ArrayList<>(entries);
        try (ObjectOutputStream oos = new ObjectOutputStream(Files.newOutputStream(Paths.get(filename)))) {
            oos.writeObject(data);
        }
    }

    public void loadFromFile(String filename) throws IOException, ClassNotFoundException {
        try (ObjectInputStream ois = new ObjectInputStream(Files.newInputStream(Paths.get(filename)))) {
            SleepData data = (SleepData) ois.readObject();
            entries = new ArrayList<>(data.entries);
            for (SleepEntry e : entries) {
                if (e.id() >= nextId) nextId = e.id() + 1;
            }
        }
    }

    public List<SleepEntry> getEntries() { return Collections.unmodifiableList(entries); }
}

public class SleepCalculatorApp {
    private static final Scanner scanner = new Scanner(System.in);

    private static String readString(String prompt) {
        System.out.print(prompt);
        return scanner.nextLine().trim();
    }

    private static int readInt(String prompt) {
        while (true) {
            try {
                System.out.print(prompt);
                return Integer.parseInt(scanner.nextLine().trim());
            } catch (NumberFormatException e) {
                System.out.println("Введите число.");
            }
        }
    }

    private static void printEntry(SleepEntry entry) {
        String stars = "⭐".repeat(entry.quality()) + "☆".repeat(5 - entry.quality());
        System.out.printf("#%d - %s | Лёг: %s | Проснулся: %s%n", entry.id(), entry.date(), entry.bedtime(), entry.wakeup());
        System.out.printf("   Продолжительность: %d мин (%dч %dм)%n", entry.duration(), entry.duration()/60, entry.duration()%60);
        System.out.printf("   Качество: %s (%d/5)%n", stars, entry.quality());
    }

    public static void main(String[] args) {
        SleepCalculator calc = new SleepCalculator();
        try {
            calc.loadFromFile("sleep_data.ser");
        } catch (IOException | ClassNotFoundException e) {
            System.out.println("Не удалось загрузить данные.");
        }

        while (true) {
            System.out.println("\n===== КАЛЬКУЛЯТОР СНА (Java) =====");
            System.out.println("1. Рассчитать время пробуждения");
            System.out.println("2. Рассчитать время засыпания");
            System.out.println("3. Добавить запись о сне");
            System.out.println("4. Показать все записи");
            System.out.println("5. Показать статистику");
            System.out.println("6. Удалить запись");
            System.out.println("7. Сохранить в файл");
            System.out.println("8. Загрузить из файла");
            System.out.println("0. Выход");
            String choice = readString("Выберите действие: ");

            switch (choice) {
                case "0" -> { return; }
                case "1" -> {
                    String bedtime = readString("Введите время засыпания (ЧЧ:ММ): ");
                    int cycles = 5;
                    String cyclesStr = readString("Введите количество циклов (по умолчанию 5): ");
                    if (!cyclesStr.isEmpty()) cycles = Integer.parseInt(cyclesStr);
                    try {
                        String wakeup = calc.calculateWakeup(bedtime, cycles);
                        System.out.println("Вы должны проснуться в: " + wakeup);
                    } catch (Exception e) {
                        System.out.println("Ошибка: " + e.getMessage());
                    }
                }
                case "2" -> {
                    String wakeup = readString("Введите время пробуждения (ЧЧ:ММ): ");
                    int cycles = 5;
                    String cyclesStr = readString("Введите количество циклов (по умолчанию 5): ");
                    if (!cyclesStr.isEmpty()) cycles = Integer.parseInt(cyclesStr);
                    try {
                        String bedtime = calc.calculateBedtime(wakeup, cycles);
                        System.out.println("Вам следует лечь спать в: " + bedtime);
                    } catch (Exception e) {
                        System.out.println("Ошибка: " + e.getMessage());
                    }
                }
                case "3" -> {
                    String bedtime = readString("Время засыпания (ЧЧ:ММ): ");
                    String wakeup = readString("Время пробуждения (ЧЧ:ММ): ");
                    int quality = readInt("Оценка качества (1-5): ");
                    try {
                        SleepEntry entry = calc.addEntry(bedtime, wakeup, quality);
                        System.out.println("Запись добавлена с ID " + entry.id());
                    } catch (Exception e) {
                        System.out.println("Ошибка: " + e.getMessage());
                    }
                }
                case "4" -> {
                    if (calc.getEntries().isEmpty()) {
                        System.out.println("Записей нет.");
                    } else {
                        calc.getEntries().forEach(SleepCalculatorApp::printEntry);
                    }
                }
                case "5" -> {
                    var stats = calc.getStats();
                    if ((int)stats.get("total") == 0) {
                        System.out.println("Нет записей для статистики.");
                    } else {
                        System.out.println("\n=== СТАТИСТИКА ===");
                        System.out.println("Всего записей: " + stats.get("total"));
                        System.out.printf("Средняя продолжительность: %.1f мин (%.0fч %.0fм)%n",
                            stats.get("avg_duration"), ((double)stats.get("avg_duration"))/60, ((double)stats.get("avg_duration"))%60);
                        System.out.printf("Средняя оценка качества: %.2f/5%n", stats.get("avg_quality"));
                    }
                }
                case "6" -> {
                    int id = readInt("Введите ID записи для удаления: ");
                    if (calc.deleteEntry(id)) {
                        System.out.println("Запись удалена.");
                    } else {
                        System.out.println("Запись не найдена.");
                    }
                }
                case "7" -> {
                    try {
                        calc.saveToFile("sleep_data.ser");
                        System.out.println("Сохранено.");
                    } catch (IOException e) {
                        System.out.println("Ошибка сохранения: " + e.getMessage());
                    }
                }
                case "8" -> {
                    try {
                        calc.loadFromFile("sleep_data.ser");
                        System.out.println("Загружено.");
                    } catch (IOException | ClassNotFoundException e) {
                        System.out.println("Ошибка загрузки: " + e.getMessage());
                    }
                }
                default -> System.out.println("Неизвестная команда.");
            }
        }
    }
}
