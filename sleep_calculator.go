// sleep_calculator.go
package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"os"
	"strconv"
	"strings"
	"time"
)

const cycleMinutes = 90

type SleepEntry struct {
	ID       int    `json:"id"`
	Date     string `json:"date"`
	Bedtime  string `json:"bedtime"`  // HH:MM
	Wakeup   string `json:"wakeup"`   // HH:MM
	Duration int    `json:"duration"` // minutes
	Quality  int    `json:"quality"`  // 1-5
}

type SleepData struct {
	Entries []SleepEntry `json:"entries"`
}

type SleepCalculator struct {
	entries []SleepEntry
	nextID  int
}

func NewSleepCalculator() *SleepCalculator {
	return &SleepCalculator{
		entries: []SleepEntry{},
		nextID:  1,
	}
}

func (c *SleepCalculator) parseTime(timeStr string) (int, int, error) {
	parts := strings.Split(timeStr, ":")
	if len(parts) != 2 {
		return 0, 0, fmt.Errorf("неверный формат, используйте ЧЧ:ММ")
	}
	h, err := strconv.Atoi(parts[0])
	if err != nil || h < 0 || h > 23 {
		return 0, 0, fmt.Errorf("неверные часы")
	}
	m, err := strconv.Atoi(parts[1])
	if err != nil || m < 0 || m > 59 {
		return 0, 0, fmt.Errorf("неверные минуты")
	}
	return h, m, nil
}

func (c *SleepCalculator) addMinutesToTime(timeStr string, minutes int) (string, error) {
	h, m, err := c.parseTime(timeStr)
	if err != nil {
		return "", err
	}
	t := time.Date(2000, 1, 1, h, m, 0, 0, time.UTC)
	t = t.Add(time.Duration(minutes) * time.Minute)
	return t.Format("15:04"), nil
}

func (c *SleepCalculator) CalculateWakeup(bedtime string, cycles int) (string, error) {
	if cycles < 1 || cycles > 10 {
		return "", fmt.Errorf("количество циклов должно быть от 1 до 10")
	}
	total := cycles * cycleMinutes
	return c.addMinutesToTime(bedtime, total)
}

func (c *SleepCalculator) CalculateBedtime(wakeup string, cycles int) (string, error) {
	if cycles < 1 || cycles > 10 {
		return "", fmt.Errorf("количество циклов должно быть от 1 до 10")
	}
	total := cycles * cycleMinutes
	h, m, err := c.parseTime(wakeup)
	if err != nil {
		return "", err
	}
	t := time.Date(2000, 1, 1, h, m, 0, 0, time.UTC)
	t = t.Add(-time.Duration(total) * time.Minute)
	return t.Format("15:04"), nil
}

func (c *SleepCalculator) AddEntry(bedtime, wakeup string, quality int) (SleepEntry, error) {
	if quality < 1 || quality > 5 {
		return SleepEntry{}, fmt.Errorf("оценка качества должна быть от 1 до 5")
	}
	h1, m1, err := c.parseTime(bedtime)
	if err != nil {
		return SleepEntry{}, err
	}
	h2, m2, err := c.parseTime(wakeup)
	if err != nil {
		return SleepEntry{}, err
	}
	t1 := time.Date(2000, 1, 1, h1, m1, 0, 0, time.UTC)
	t2 := time.Date(2000, 1, 1, h2, m2, 0, 0, time.UTC)
	if t2.Before(t1) || t2.Equal(t1) {
		t2 = t2.Add(24 * time.Hour)
	}
	duration := int(t2.Sub(t1).Minutes())
	entry := SleepEntry{
		ID:       c.nextID,
		Date:     time.Now().Format("2006-01-02"),
		Bedtime:  bedtime,
		Wakeup:   wakeup,
		Duration: duration,
		Quality:  quality,
	}
	c.entries = append(c.entries, entry)
	c.nextID++
	return entry, nil
}

func (c *SleepCalculator) FindEntry(id int) *SleepEntry {
	for i := range c.entries {
		if c.entries[i].ID == id {
			return &c.entries[i]
		}
	}
	return nil
}

func (c *SleepCalculator) DeleteEntry(id int) bool {
	for i, e := range c.entries {
		if e.ID == id {
			c.entries = append(c.entries[:i], c.entries[i+1:]...)
			return true
		}
	}
	return false
}

func (c *SleepCalculator) Stats() map[string]interface{} {
	total := len(c.entries)
	if total == 0 {
		return map[string]interface{}{"total": 0, "avg_duration": nil, "avg_quality": nil}
	}
	sumDur := 0
	sumQual := 0
	for _, e := range c.entries {
		sumDur += e.Duration
		sumQual += e.Quality
	}
	return map[string]interface{}{
		"total":        total,
		"avg_duration": float64(sumDur) / float64(total),
		"avg_quality":  float64(sumQual) / float64(total),
	}
}

func (c *SleepCalculator) SaveToFile(filename string) error {
	data := SleepData{Entries: c.entries}
	jsonData, err := json.MarshalIndent(data, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(filename, jsonData, 0644)
}

func (c *SleepCalculator) LoadFromFile(filename string) error {
	data, err := os.ReadFile(filename)
	if err != nil {
		if os.IsNotExist(err) {
			return nil
		}
		return err
	}
	var sd SleepData
	if err := json.Unmarshal(data, &sd); err != nil {
		return err
	}
	c.entries = sd.Entries
	for _, e := range c.entries {
		if e.ID >= c.nextID {
			c.nextID = e.ID + 1
		}
	}
	return nil
}

func readString(prompt string) string {
	fmt.Print(prompt)
	reader := bufio.NewReader(os.Stdin)
	input, _ := reader.ReadString('\n')
	return strings.TrimSpace(input)
}

func readInt(prompt string) int {
	for {
		input := readString(prompt)
		if val, err := strconv.Atoi(input); err == nil {
			return val
		}
		fmt.Println("Введите число.")
	}
}

func printEntry(entry SleepEntry) {
	stars := strings.Repeat("⭐", entry.Quality) + strings.Repeat("☆", 5-entry.Quality)
	fmt.Printf("#%d - %s | Лёг: %s | Проснулся: %s\n", entry.ID, entry.Date, entry.Bedtime, entry.Wakeup)
	fmt.Printf("   Продолжительность: %d мин (%dч %dм)\n", entry.Duration, entry.Duration/60, entry.Duration%60)
	fmt.Printf("   Качество: %s (%d/5)\n", stars, entry.Quality)
}

func main() {
	calc := NewSleepCalculator()
	if err := calc.LoadFromFile("sleep_data.json"); err != nil {
		fmt.Println("Ошибка загрузки:", err)
	}

	for {
		fmt.Println("\n===== КАЛЬКУЛЯТОР СНА (Go) =====")
		fmt.Println("1. Рассчитать время пробуждения")
		fmt.Println("2. Рассчитать время засыпания")
		fmt.Println("3. Добавить запись о сне")
		fmt.Println("4. Показать все записи")
		fmt.Println("5. Показать статистику")
		fmt.Println("6. Удалить запись")
		fmt.Println("7. Сохранить в файл")
		fmt.Println("8. Загрузить из файла")
		fmt.Println("0. Выход")
		choice := readString("Выберите действие: ")

		switch choice {
		case "0":
			return
		case "1":
			bedtime := readString("Введите время засыпания (ЧЧ:ММ): ")
			cycles := 5
			if input := readString("Введите количество циклов (по умолчанию 5): "); input != "" {
				cycles, _ = strconv.Atoi(input)
			}
			if wakeup, err := calc.CalculateWakeup(bedtime, cycles); err != nil {
				fmt.Println("Ошибка:", err)
			} else {
				fmt.Printf("Вы должны проснуться в: %s\n", wakeup)
			}
		case "2":
			wakeup := readString("Введите время пробуждения (ЧЧ:ММ): ")
			cycles := 5
			if input := readString("Введите количество циклов (по умолчанию 5): "); input != "" {
				cycles, _ = strconv.Atoi(input)
			}
			if bedtime, err := calc.CalculateBedtime(wakeup, cycles); err != nil {
				fmt.Println("Ошибка:", err)
			} else {
				fmt.Printf("Вам следует лечь спать в: %s\n", bedtime)
			}
		case "3":
			bedtime := readString("Время засыпания (ЧЧ:ММ): ")
			wakeup := readString("Время пробуждения (ЧЧ:ММ): ")
			quality := readInt("Оценка качества (1-5): ")
			entry, err := calc.AddEntry(bedtime, wakeup, quality)
			if err != nil {
				fmt.Println("Ошибка:", err)
			} else {
				fmt.Printf("Запись добавлена с ID %d\n", entry.ID)
			}
		case "4":
			if len(calc.entries) == 0 {
				fmt.Println("Записей нет.")
			} else {
				for _, e := range calc.entries {
					printEntry(e)
				}
			}
		case "5":
			stats := calc.Stats()
			if stats["total"] == 0 {
				fmt.Println("Нет записей для статистики.")
			} else {
				fmt.Println("\n=== СТАТИСТИКА ===")
				fmt.Printf("Всего записей: %d\n", stats["total"])
				fmt.Printf("Средняя продолжительность: %.1f мин (%.0fч %.0fм)\n",
					stats["avg_duration"], stats["avg_duration"].(float64)/60, math.Mod(stats["avg_duration"].(float64), 60))
				fmt.Printf("Средняя оценка качества: %.2f/5\n", stats["avg_quality"])
			}
		case "6":
			id := readInt("Введите ID записи для удаления: ")
			if calc.DeleteEntry(id) {
				fmt.Println("Запись удалена.")
			} else {
				fmt.Println("Запись не найдена.")
			}
		case "7":
			if err := calc.SaveToFile("sleep_data.json"); err != nil {
				fmt.Println("Ошибка сохранения:", err)
			} else {
				fmt.Println("Сохранено.")
			}
		case "8":
			if err := calc.LoadFromFile("sleep_data.json"); err != nil {
				fmt.Println("Ошибка загрузки:", err)
			} else {
				fmt.Println("Загружено.")
			}
		default:
			fmt.Println("Неизвестная команда.")
		}
	}
}
