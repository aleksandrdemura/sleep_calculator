// sleep_calculator.js
const fs = require('fs').promises;
const readline = require('readline');

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

const question = (prompt) => new Promise(resolve => rl.question(prompt, resolve));

const CYCLE_MINUTES = 90;

class SleepEntry {
    constructor(id, date, bedtime, wakeup, duration, quality) {
        this.id = id;
        this.date = date;
        this.bedtime = bedtime;
        this.wakeup = wakeup;
        this.duration = duration;
        this.quality = quality;
    }
}

class SleepCalculator {
    constructor() {
        this.entries = [];
        this.nextId = 1;
    }

    parseTime(timeStr) {
        const parts = timeStr.split(':');
        if (parts.length !== 2) throw new Error('Неверный формат, используйте ЧЧ:ММ');
        const h = parseInt(parts[0]);
        const m = parseInt(parts[1]);
        if (isNaN(h) || isNaN(m) || h < 0 || h > 23 || m < 0 || m > 59)
            throw new Error('Неверные часы или минуты');
        return { h, m };
    }

    addMinutesToTime(timeStr, minutes) {
        const { h, m } = this.parseTime(timeStr);
        const dt = new Date(2000, 0, 1, h, m);
        dt.setMinutes(dt.getMinutes() + minutes);
        return String(dt.getHours()).padStart(2, '0') + ':' + String(dt.getMinutes()).padStart(2, '0');
    }

    calculateWakeup(bedtime, cycles = 5) {
        if (cycles < 1 || cycles > 10) throw new Error('Количество циклов должно быть от 1 до 10');
        return this.addMinutesToTime(bedtime, cycles * CYCLE_MINUTES);
    }

    calculateBedtime(wakeup, cycles = 5) {
        if (cycles < 1 || cycles > 10) throw new Error('Количество циклов должно быть от 1 до 10');
        const total = cycles * CYCLE_MINUTES;
        const { h, m } = this.parseTime(wakeup);
        const dt = new Date(2000, 0, 1, h, m);
        dt.setMinutes(dt.getMinutes() - total);
        return String(dt.getHours()).padStart(2, '0') + ':' + String(dt.getMinutes()).padStart(2, '0');
    }

    addEntry(bedtime, wakeup, quality) {
        if (quality < 1 || quality > 5) throw new Error('Оценка качества должна быть от 1 до 5');
        const t1 = this.parseTime(bedtime);
        const t2 = this.parseTime(wakeup);
        let dt1 = new Date(2000, 0, 1, t1.h, t1.m);
        let dt2 = new Date(2000, 0, 1, t2.h, t2.m);
        if (dt2 <= dt1) dt2.setDate(dt2.getDate() + 1);
        const duration = Math.round((dt2 - dt1) / 60000);
        const entry = new SleepEntry(
            this.nextId,
            new Date().toISOString().slice(0,10),
            bedtime,
            wakeup,
            duration,
            quality
        );
        this.entries.push(entry);
        this.nextId++;
        return entry;
    }

    findEntry(id) {
        return this.entries.find(e => e.id === id);
    }

    deleteEntry(id) {
        const index = this.entries.findIndex(e => e.id === id);
        if (index === -1) return false;
        this.entries.splice(index, 1);
        return true;
    }

    getStats() {
        const total = this.entries.length;
        if (total === 0) return { total: 0, avgDuration: null, avgQuality: null };
        const sumDur = this.entries.reduce((s, e) => s + e.duration, 0);
        const sumQual = this.entries.reduce((s, e) => s + e.quality, 0);
        return {
            total,
            avgDuration: sumDur / total,
            avgQuality: sumQual / total
        };
    }

    async saveToFile(filename = 'sleep_data.json') {
        const data = { entries: this.entries };
        await fs.writeFile(filename, JSON.stringify(data, null, 2));
    }

    async loadFromFile(filename = 'sleep_data.json') {
        try {
            const data = await fs.readFile(filename, 'utf8');
            const parsed = JSON.parse(data);
            this.entries = parsed.entries.map(e => Object.assign(new SleepEntry(0), e));
            this.nextId = this.entries.reduce((max, e) => Math.max(max, e.id), 0) + 1;
        } catch (err) {
            if (err.code !== 'ENOENT') throw err;
        }
    }
}

function printEntry(entry) {
    const stars = '⭐'.repeat(entry.quality) + '☆'.repeat(5 - entry.quality);
    console.log(`#${entry.id} - ${entry.date} | Лёг: ${entry.bedtime} | Проснулся: ${entry.wakeup}`);
    console.log(`   Продолжительность: ${entry.duration} мин (${Math.floor(entry.duration/60)}ч ${entry.duration%60}м)`);
    console.log(`   Качество: ${stars} (${entry.quality}/5)`);
}

async function main() {
    const calc = new SleepCalculator();
    await calc.loadFromFile();

    while (true) {
        console.log('\n===== КАЛЬКУЛЯТОР СНА (JavaScript) =====');
        console.log('1. Рассчитать время пробуждения');
        console.log('2. Рассчитать время засыпания');
        console.log('3. Добавить запись о сне');
        console.log('4. Показать все записи');
        console.log('5. Показать статистику');
        console.log('6. Удалить запись');
        console.log('7. Сохранить в файл');
        console.log('8. Загрузить из файла');
        console.log('0. Выход');
        const choice = await question('Выберите действие: ');

        if (choice === '0') break;

        switch (choice) {
            case '1': {
                const bedtime = await question('Введите время засыпания (ЧЧ:ММ): ');
                let cyclesInput = await question('Введите количество циклов (по умолчанию 5): ');
                let cycles = cyclesInput.trim() === '' ? 5 : parseInt(cyclesInput);
                try {
                    const wakeup = calc.calculateWakeup(bedtime, cycles);
                    console.log(`Вы должны проснуться в: ${wakeup}`);
                } catch (err) {
                    console.log('Ошибка:', err.message);
                }
                break;
            }
            case '2': {
                const wakeup = await question('Введите время пробуждения (ЧЧ:ММ): ');
                let cyclesInput = await question('Введите количество циклов (по умолчанию 5): ');
                let cycles = cyclesInput.trim() === '' ? 5 : parseInt(cyclesInput);
                try {
                    const bedtime = calc.calculateBedtime(wakeup, cycles);
                    console.log(`Вам следует лечь спать в: ${bedtime}`);
                } catch (err) {
                    console.log('Ошибка:', err.message);
                }
                break;
            }
            case '3': {
                const bedtime = await question('Время засыпания (ЧЧ:ММ): ');
                const wakeup = await question('Время пробуждения (ЧЧ:ММ): ');
                const quality = parseInt(await question('Оценка качества (1-5): '));
                try {
                    const entry = calc.addEntry(bedtime, wakeup, quality);
                    console.log(`Запись добавлена с ID ${entry.id}`);
                } catch (err) {
                    console.log('Ошибка:', err.message);
                }
                break;
            }
            case '4':
                if (calc.entries.length === 0) {
                    console.log('Записей нет.');
                } else {
                    calc.entries.forEach(printEntry);
                }
                break;
            case '5': {
                const stats = calc.getStats();
                if (stats.total === 0) {
                    console.log('Нет записей для статистики.');
                } else {
                    console.log('\n=== СТАТИСТИКА ===');
                    console.log(`Всего записей: ${stats.total}`);
                    console.log(`Средняя продолжительность: ${stats.avgDuration.toFixed(1)} мин (${Math.floor(stats.avgDuration/60)}ч ${Math.round(stats.avgDuration%60)}м)`);
                    console.log(`Средняя оценка качества: ${stats.avgQuality.toFixed(2)}/5`);
                }
                break;
            }
            case '6': {
                const id = parseInt(await question('Введите ID записи для удаления: '));
                if (calc.deleteEntry(id)) {
                    console.log('Запись удалена.');
                } else {
                    console.log('Запись не найдена.');
                }
                break;
            }
            case '7':
                try {
                    await calc.saveToFile();
                    console.log('Сохранено.');
                } catch (err) {
                    console.log('Ошибка сохранения:', err.message);
                }
                break;
            case '8':
                try {
                    await calc.loadFromFile();
                    console.log('Загружено.');
                } catch (err) {
                    console.log('Ошибка загрузки:', err.message);
                }
                break;
            default:
                console.log('Неизвестная команда.');
        }
    }
    rl.close();
}

main().catch(console.error);
