// sleep_calculator.cpp
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <map>
#include <variant>
#include <regex>
#include <cmath>

using namespace std;

const int CYCLE_MINUTES = 90;

struct SleepEntry {
    int id;
    string date;
    string bedtime;
    string wakeup;
    int duration;
    int quality;

    SleepEntry(int id, const string& date, const string& bedtime, const string& wakeup, int duration, int quality)
        : id(id), date(date), bedtime(bedtime), wakeup(wakeup), duration(duration), quality(quality) {}
};

class SleepCalculator {
private:
    vector<SleepEntry> entries;
    int nextId = 1;

    pair<int, int> parseTime(const string& timeStr) {
        regex pattern(R"(^(\d{2}):(\d{2})$)");
        smatch matches;
        if (!regex_search(timeStr, matches, pattern)) {
            throw invalid_argument("Неверный формат времени, используйте ЧЧ:ММ");
        }
        int h = stoi(matches[1]);
        int m = stoi(matches[2]);
        if (h < 0 || h > 23 || m < 0 || m > 59) {
            throw invalid_argument("Неверные часы или минуты");
        }
        return {h, m};
    }

    string addMinutesToTime(const string& timeStr, int minutes) {
        auto [h, m] = parseTime(timeStr);
        tm t = {};
        t.tm_hour = h;
        t.tm_min = m;
        t.tm_sec = 0;
        time_t raw = mktime(&t);
        raw += minutes * 60;
        tm* result = localtime(&raw);
        char buf[6];
        strftime(buf, sizeof(buf), "%H:%M", result);
        return string(buf);
    }

public:
    string calculateWakeup(const string& bedtime, int cycles = 5) {
        if (cycles < 1 || cycles > 10) throw invalid_argument("Количество циклов должно быть от 1 до 10");
        return addMinutesToTime(bedtime, cycles * CYCLE_MINUTES);
    }

    string calculateBedtime(const string& wakeup, int cycles = 5) {
        if (cycles < 1 || cycles > 10) throw invalid_argument("Количество циклов должно быть от 1 до 10");
        auto [h, m] = parseTime(wakeup);
        tm t = {};
        t.tm_hour = h;
        t.tm_min = m;
        t.tm_sec = 0;
        time_t raw = mktime(&t);
        raw -= cycles * CYCLE_MINUTES * 60;
        tm* result = localtime(&raw);
        char buf[6];
        strftime(buf, sizeof(buf), "%H:%M", result);
        return string(buf);
    }

    SleepEntry addEntry(const string& bedtime, const string& wakeup, int quality) {
        if (quality < 1 || quality > 5) throw invalid_argument("Оценка качества должна быть от 1 до 5");
        auto [h1, m1] = parseTime(bedtime);
        auto [h2, m2] = parseTime(wakeup);
        tm t1 = {}, t2 = {};
        t1.tm_hour = h1; t1.tm_min = m1;
        t2.tm_hour = h2; t2.tm_min = m2;
        time_t raw1 = mktime(&t1);
        time_t raw2 = mktime(&t2);
        if (raw2 <= raw1) raw2 += 24 * 3600;
        int duration = (raw2 - raw1) / 60;
        time_t now = time(nullptr);
        tm* now_tm = localtime(&now);
        char dateBuf[11];
        strftime(dateBuf, sizeof(dateBuf), "%Y-%m-%d", now_tm);
        SleepEntry entry(nextId, string(dateBuf), bedtime, wakeup, duration, quality);
        entries.push_back(entry);
        nextId++;
        return entry;
    }

    SleepEntry* findEntry(int id) {
        auto it = find_if(entries.begin(), entries.end(), [id](const SleepEntry& e) { return e.id == id; });
        return it != entries.end() ? &(*it) : nullptr;
    }

    bool deleteEntry(int id) {
        auto it = find_if(entries.begin(), entries.end(), [id](const SleepEntry& e) { return e.id == id; });
        if (it == entries.end()) return false;
        entries.erase(it);
        return true;
    }

    map<string, variant<int, double>> getStats() {
        int total = entries.size();
        if (total == 0) {
            return {{"total", 0}, {"avg_duration", nullptr}, {"avg_quality", nullptr}};
        }
        int sumDur = 0, sumQual = 0;
        for (const auto& e : entries) {
            sumDur += e.duration;
            sumQual += e.quality;
        }
        return {
            {"total", total},
            {"avg_duration", static_cast<double>(sumDur) / total},
            {"avg_quality", static_cast<double>(sumQual) / total}
        };
    }

    void saveToFile(const string& filename = "sleep_data.txt") {
        ofstream out(filename);
        if (!out) return;
        for (const auto& e : entries) {
            out << e.id << '|'
                << e.date << '|'
                << e.bedtime << '|'
                << e.wakeup << '|'
                << e.duration << '|'
                << e.quality << '\n';
        }
    }

    void loadFromFile(const string& filename = "sleep_data.txt") {
        ifstream in(filename);
        if (!in) return;
        entries.clear();
        string line;
        while (getline(in, line)) {
            stringstream ss(line);
            string idStr, date, bedtime, wakeup, durStr, qualStr;
            getline(ss, idStr, '|');
            getline(ss, date, '|');
            getline(ss, bedtime, '|');
            getline(ss, wakeup, '|');
            getline(ss, durStr, '|');
            getline(ss, qualStr, '|');
            int id = stoi(idStr);
            int duration = stoi(durStr);
            int quality = stoi(qualStr);
            entries.emplace_back(id, date, bedtime, wakeup, duration, quality);
            if (id >= nextId) nextId = id + 1;
        }
    }

    const vector<SleepEntry>& getEntries() const { return entries; }
};

string readString(const string& prompt) {
    cout << prompt;
    string input;
    getline(cin, input);
    return input;
}

int readInt(const string& prompt) {
    while (true) {
        cout << prompt;
        string input;
        getline(cin, input);
        try {
            return stoi(input);
        } catch (...) {
            cout << "Введите число.\n";
        }
    }
}

void printEntry(const SleepEntry& entry) {
    string stars(entry.quality, '⭐');
    stars += string(5 - entry.quality, '☆');
    cout << "#" << entry.id << " - " << entry.date << " | Лёг: " << entry.bedtime << " | Проснулся: " << entry.wakeup << "\n";
    cout << "   Продолжительность: " << entry.duration << " мин (" << entry.duration/60 << "ч " << entry.duration%60 << "м)\n";
    cout << "   Качество: " << stars << " (" << entry.quality << "/5)\n";
}

int main() {
    SleepCalculator calc;
    calc.loadFromFile();

    while (true) {
        cout << "\n===== КАЛЬКУЛЯТОР СНА (C++) =====" << endl;
        cout << "1. Рассчитать время пробуждения\n";
        cout << "2. Рассчитать время засыпания\n";
        cout << "3. Добавить запись о сне\n";
        cout << "4. Показать все записи\n";
        cout << "5. Показать статистику\n";
        cout << "6. Удалить запись\n";
        cout << "7. Сохранить в файл\n";
        cout << "8. Загрузить из файла\n";
        cout << "0. Выход\n";
        string choice = readString("Выберите действие: ");

        if (choice == "0") break;

        if (choice == "1") {
            string bedtime = readString("Введите время засыпания (ЧЧ:ММ): ");
            string cyclesInput = readString("Введите количество циклов (по умолчанию 5): ");
            int cycles = cyclesInput.empty() ? 5 : stoi(cyclesInput);
            try {
                string wakeup = calc.calculateWakeup(bedtime, cycles);
                cout << "Вы должны проснуться в: " << wakeup << "\n";
            } catch (const exception& e) {
                cout << "Ошибка: " << e.what() << "\n";
            }
        } else if (choice == "2") {
            string wakeup = readString("Введите время пробуждения (ЧЧ:ММ): ");
            string cyclesInput = readString("Введите количество циклов (по умолчанию 5): ");
            int cycles = cyclesInput.empty() ? 5 : stoi(cyclesInput);
            try {
                string bedtime = calc.calculateBedtime(wakeup, cycles);
                cout << "Вам следует лечь спать в: " << bedtime << "\n";
            } catch (const exception& e) {
                cout << "Ошибка: " << e.what() << "\n";
            }
        } else if (choice == "3") {
            string bedtime = readString("Время засыпания (ЧЧ:ММ): ");
            string wakeup = readString("Время пробуждения (ЧЧ:ММ): ");
            int quality = readInt("Оценка качества (1-5): ");
            try {
                SleepEntry entry = calc.addEntry(bedtime, wakeup, quality);
                cout << "Запись добавлена с ID " << entry.id << "\n";
            } catch (const exception& e) {
                cout << "Ошибка: " << e.what() << "\n";
            }
        } else if (choice == "4") {
            if (calc.getEntries().empty()) {
                cout << "Записей нет.\n";
            } else {
                for (const auto& e : calc.getEntries()) {
                    printEntry(e);
                }
            }
        } else if (choice == "5") {
            auto stats = calc.getStats();
            if (get<int>(stats["total"]) == 0) {
                cout << "Нет записей для статистики.\n";
            } else {
                cout << "\n=== СТАТИСТИКА ===\n";
                cout << "Всего записей: " << get<int>(stats["total"]) << "\n";
                double avgDur = get<double>(stats["avg_duration"]);
                cout << "Средняя продолжительность: " << fixed << setprecision(1) << avgDur << " мин ("
                     << (int)(avgDur/60) << "ч " << (int)round(avgDur) % 60 << "м)\n";
                cout << "Средняя оценка качества: " << fixed << setprecision(2) << get<double>(stats["avg_quality"]) << "/5\n";
            }
        } else if (choice == "6") {
            int id = readInt("Введите ID записи для удаления: ");
            if (calc.deleteEntry(id)) {
                cout << "Запись удалена.\n";
            } else {
                cout << "Запись не найдена.\n";
            }
        } else if (choice == "7") {
            calc.saveToFile();
            cout << "Сохранено.\n";
        } else if (choice == "8") {
            calc.loadFromFile();
            cout << "Загружено.\n";
        } else {
            cout << "Неизвестная команда.\n";
        }
    }
    return 0;
}
