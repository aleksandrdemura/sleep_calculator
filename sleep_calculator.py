# sleep_calculator.py
import json
from dataclasses import dataclass, asdict
from datetime import datetime, timedelta
from typing import List, Optional
from pathlib import Path

CYCLE_MINUTES = 90

@dataclass
class SleepEntry:
    id: int
    date: str
    bedtime: str   # HH:MM
    wakeup: str    # HH:MM
    duration: int  # минуты
    quality: int   # 1-5

class SleepCalculator:
    def __init__(self):
        self.entries: List[SleepEntry] = []
        self.next_id = 1

    @staticmethod
    def parse_time(time_str: str) -> tuple:
        try:
            h, m = map(int, time_str.split(':'))
            if 0 <= h < 24 and 0 <= m < 60:
                return h, m
        except:
            pass
        raise ValueError("Неверный формат времени, используйте ЧЧ:ММ")

    @staticmethod
    def add_minutes_to_time(time_str: str, minutes: int) -> str:
        h, m = SleepCalculator.parse_time(time_str)
        dt = datetime(2000, 1, 1, h, m) + timedelta(minutes=minutes)
        return dt.strftime("%H:%M")

    def calculate_wakeup(self, bedtime: str, cycles: int = 5) -> str:
        if cycles < 1 or cycles > 10:
            raise ValueError("Количество циклов должно быть от 1 до 10")
        total_minutes = cycles * CYCLE_MINUTES
        return self.add_minutes_to_time(bedtime, total_minutes)

    def calculate_bedtime(self, wakeup: str, cycles: int = 5) -> str:
        if cycles < 1 or cycles > 10:
            raise ValueError("Количество циклов должно быть от 1 до 10")
        total_minutes = cycles * CYCLE_MINUTES
        h, m = self.parse_time(wakeup)
        dt = datetime(2000, 1, 1, h, m) - timedelta(minutes=total_minutes)
        return dt.strftime("%H:%M")

    def add_entry(self, bedtime: str, wakeup: str, quality: int) -> SleepEntry:
        self.parse_time(bedtime)
        self.parse_time(wakeup)
        if quality < 1 or quality > 5:
            raise ValueError("Оценка качества должна быть от 1 до 5")
        h1, m1 = self.parse_time(bedtime)
        h2, m2 = self.parse_time(wakeup)
        # Вычисляем продолжительность в минутах (может быть переход через полночь)
        dt1 = datetime(2000, 1, 1, h1, m1)
        dt2 = datetime(2000, 1, 1, h2, m2)
        if dt2 <= dt1:
            dt2 += timedelta(days=1)
        duration = int((dt2 - dt1).total_seconds() // 60)
        entry = SleepEntry(
            id=self.next_id,
            date=datetime.now().strftime("%Y-%m-%d"),
            bedtime=bedtime,
            wakeup=wakeup,
            duration=duration,
            quality=quality
        )
        self.entries.append(entry)
        self.next_id += 1
        return entry

    def find_entry(self, entry_id: int) -> Optional[SleepEntry]:
        return next((e for e in self.entries if e.id == entry_id), None)

    def delete_entry(self, entry_id: int) -> bool:
        entry = self.find_entry(entry_id)
        if entry:
            self.entries.remove(entry)
            return True
        return False

    def get_stats(self) -> dict:
        total = len(self.entries)
        if total == 0:
            return {"total": 0, "avg_duration": None, "avg_quality": None}
        durations = [e.duration for e in self.entries]
        qualities = [e.quality for e in self.entries]
        return {
            "total": total,
            "avg_duration": sum(durations) / total,
            "avg_quality": sum(qualities) / total
        }

    def save_to_file(self, filename: str = "sleep_data.json") -> None:
        data = {"entries": [asdict(e) for e in self.entries]}
        with open(filename, "w", encoding="utf-8") as f:
            json.dump(data, f, ensure_ascii=False, indent=2)

    def load_from_file(self, filename: str = "sleep_data.json") -> None:
        path = Path(filename)
        if not path.exists():
            return
        with open(filename, "r", encoding="utf-8") as f:
            data = json.load(f)
            self.entries.clear()
            for item in data.get("entries", []):
                entry = SleepEntry(
                    id=item["id"],
                    date=item["date"],
                    bedtime=item["bedtime"],
                    wakeup=item["wakeup"],
                    duration=item["duration"],
                    quality=item["quality"]
                )
                self.entries.append(entry)
                if entry.id >= self.next_id:
                    self.next_id = entry.id + 1

def print_entry(entry: SleepEntry) -> None:
    stars = "⭐" * entry.quality + "☆" * (5 - entry.quality)
    print(f"#{entry.id} - {entry.date} | Лёг: {entry.bedtime} | Проснулся: {entry.wakeup}")
    print(f"   Продолжительность: {entry.duration} мин ({entry.duration//60}ч {entry.duration%60}м)")
    print(f"   Качество: {stars} ({entry.quality}/5)")

def main():
    calc = SleepCalculator()
    calc.load_from_file()

    while True:
        print("\n===== КАЛЬКУЛЯТОР СНА (Python) =====")
        print("1. Рассчитать время пробуждения")
        print("2. Рассчитать время засыпания")
        print("3. Добавить запись о сне")
        print("4. Показать все записи")
        print("5. Показать статистику")
        print("6. Удалить запись")
        print("7. Сохранить в файл")
        print("8. Загрузить из файла")
        print("0. Выход")
        choice = input("Выберите действие: ").strip()

        if choice == "0":
            break
        elif choice == "1":
            bedtime = input("Введите время засыпания (ЧЧ:ММ): ").strip()
            try:
                cycles = int(input("Введите количество циклов (по умолчанию 5): ").strip() or "5")
            except ValueError:
                cycles = 5
            try:
                wakeup = calc.calculate_wakeup(bedtime, cycles)
                print(f"Вы должны проснуться в: {wakeup}")
            except Exception as e:
                print("Ошибка:", e)
        elif choice == "2":
            wakeup = input("Введите время пробуждения (ЧЧ:ММ): ").strip()
            try:
                cycles = int(input("Введите количество циклов (по умолчанию 5): ").strip() or "5")
            except ValueError:
                cycles = 5
            try:
                bedtime = calc.calculate_bedtime(wakeup, cycles)
                print(f"Вам следует лечь спать в: {bedtime}")
            except Exception as e:
                print("Ошибка:", e)
        elif choice == "3":
            bedtime = input("Время засыпания (ЧЧ:ММ): ").strip()
            wakeup = input("Время пробуждения (ЧЧ:ММ): ").strip()
            try:
                quality = int(input("Оценка качества (1-5): ").strip())
            except ValueError:
                quality = 3
            try:
                entry = calc.add_entry(bedtime, wakeup, quality)
                print(f"Запись добавлена с ID {entry.id}")
            except Exception as e:
                print("Ошибка:", e)
        elif choice == "4":
            if not calc.entries:
                print("Записей нет.")
            else:
                for e in calc.entries:
                    print_entry(e)
        elif choice == "5":
            stats = calc.get_stats()
            if stats["total"] == 0:
                print("Нет записей для статистики.")
            else:
                print("\n=== СТАТИСТИКА ===")
                print(f"Всего записей: {stats['total']}")
                print(f"Средняя продолжительность: {stats['avg_duration']:.1f} мин ({stats['avg_duration']//60:.0f}ч {stats['avg_duration']%60:.0f}м)")
                print(f"Средняя оценка качества: {stats['avg_quality']:.2f}/5")
        elif choice == "6":
            try:
                eid = int(input("Введите ID записи для удаления: ").strip())
            except ValueError:
                print("Некорректный ID.")
                continue
            if calc.delete_entry(eid):
                print("Запись удалена.")
            else:
                print("Запись не найдена.")
        elif choice == "7":
            calc.save_to_file()
            print("Сохранено.")
        elif choice == "8":
            calc.load_from_file()
            print("Загружено.")
        else:
            print("Неизвестная команда.")

if __name__ == "__main__":
    main()
