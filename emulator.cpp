#include "emulator.h"

Emulator::Emulator()
{
	cpu.init(&memory);
	display.init(&memory);
}

// Начало эмуляции CPU
void Emulator::run()
{
	sf::Time time;

	while (display.window.isOpen())
	{
		// CPU cycles to emulate per frame draw
		float cycles_per_frame = cpu.CLOCK_SPEED / framerate;
		float time_between_frames = 1000 / framerate;
		// Текущий цикл в кадре
		int current_cycle = 0;

		handle_events();

		while (current_cycle < cycles_per_frame)
		{
			Opcode code = memory.read(cpu.reg_PC);

			cpu.parse_opcode(code);
			current_cycle += cpu.num_cycles;

			update_timers(cpu.num_cycles);
			update_scanline(cpu.num_cycles);
			do_interrupts();

			cpu.num_cycles = 0;
		}

		//display.render();
		//cout << "frame " << current_cycle << endl;
		current_cycle = 0;

		int frame_time = time.asMilliseconds();

		float sleep_time = time_between_frames - frame_time;
		if (frame_time < time_between_frames)
			sf::sleep(sf::milliseconds(sleep_time));
		time = time.Zero;
		//cout << display.scanlines_rendered << endl;
		display.scanlines_rendered = 0;
	}
}

// Обработка событий окна и ввода-вывода
void Emulator::handle_events()
{
	sf::Event event;

	while (display.window.pollEvent(event))
	{
		switch (event.type)
		{
		case sf::Event::Closed:
			display.window.close();
			break;
		case sf::Event::KeyPressed:
			key_pressed(event.key.code);
			break;
		case sf::Event::KeyReleased:
			key_released(event.key.code);
			break;
		}
	}
}

void Emulator::key_pressed(Key key)
{
	// Функциональные клавиши F1-F12
	if (key >= 85 && key <= 96)
	{
		int id = key - 84;
		if (sf::Keyboard::isKeyPressed(Key::LShift))
			save_state(id);
		else
			load_state(id);
		return;
	}

	if (key == Key::Space)
	{
		cpu.CLOCK_SPEED *= 100;
		return;
	}

	int key_id = get_key_id(key);

	if (key_id < 0)
		return;

	bool directional = false;

	if (key == Key::Up || key == Key::Down || key == Key::Left || key == Key::Right)
	{
		directional = true;
	}

	Byte joypad = (directional) ? memory.joypad_arrows : memory.joypad_buttons;
	bool unpressed = is_bit_set(joypad, key_id);

	if (!unpressed)
		return;

	if (directional)
		memory.joypad_arrows = clear_bit(joypad, key_id);
	else
		memory.joypad_buttons = clear_bit(joypad, key_id);

	request_interrupt(INTERRUPT_JOYPAD);
}

void Emulator::key_released(Key key)
{
	if (key == Key::Space)
	{
		cpu.CLOCK_SPEED /= 100;
	}

	int key_id = get_key_id(key);

	if (key_id < 0)
		return;

	bool directional = false;

	if (key == Key::Up || key == Key::Down || key == Key::Left || key == Key::Right)
	{
		directional = true;
	}

	Byte joypad = (directional) ? memory.joypad_arrows : memory.joypad_buttons;
	bool unpressed = is_bit_set(joypad, key_id);

	if (unpressed)
		return;

	if (directional)
		memory.joypad_arrows = set_bit(joypad, key_id);
	else
		memory.joypad_buttons = set_bit(joypad, key_id);
}

int Emulator::get_key_id(Key key)
{
	switch (key)
	{
	case Key::A:
	case Key::Right:
		return BIT_0;
	case Key::S: // B
	case Key::Left:
		return BIT_1;
	case Key::X: // select
	case Key::Up:
		return BIT_2;
	case Key::Z:
	case Key::Down:
		return BIT_3;
	default:
		return -1;
	}
}

void Emulator::update_divider(int cycles)
{
	divider_counter += cycles;

	if (divider_counter >= 256) // 16384 Hz
	{
		divider_counter = 0;
		memory.DIV.set(memory.DIV.get() + 1);
	}
}

// Число циклов opcode может потребовать коррекции, используются значения Nintendo
void Emulator::update_timers(int cycles)
{
	update_divider(cycles);

	// Это можно оптимизировать при необходимости
	Byte new_freq = get_timer_frequency();

	if (timer_frequency != new_freq)
	{
		set_timer_frequency();
		timer_frequency = new_freq;
	}

	if (timer_enabled())
	{
		timer_counter -= cycles;

		// достаточно тактовых циклов прошло для обновления таймера
		if (timer_counter <= 0)
		{
			Byte timer_value = memory.TIMA.get();
			set_timer_frequency();

			// Таймер переполнится, генерировать прерывание
			if (timer_value == 255)
			{
				memory.TIMA.set(memory.TMA.get());
				request_interrupt(INTERRUPT_TIMER);
			}
			else
			{
				memory.TIMA.set(timer_value + 1);
			}
		}
	}
}

bool Emulator::timer_enabled()
{
	return memory.TAC.is_bit_set(BIT_2);
}

Byte Emulator::get_timer_frequency()
{
	return (memory.TAC.get() & 0x3);
}

void Emulator::set_timer_frequency()
{
	Byte frequency = get_timer_frequency();
	timer_frequency = frequency;

	switch (frequency)
	{
		// timer_counter calculated by (Clock Speed / selected frequency)
	case 0: timer_counter = 1024; break; // 4096 Hz
	case 1: timer_counter = 16; break; // 262144 Hz
	case 2: timer_counter = 64; break; // 65536 Hz
	case 3: timer_counter = 256; break; // 16384 Hz
	}
}

void Emulator::request_interrupt(Byte id)
{
	memory.IF.set_bit(id);
}

void Emulator::do_interrupts()
{
	// Если есть установленные прерывания
	if (memory.IF.get() > 0)
	{
		// Возобновить состояние CPU, если оно приостановлено, и есть ожидающие прерывания
		if (memory.IE.get() > 0)
		{
			if (cpu.halted)
			{
				cpu.halted = false;
				cpu.reg_PC += 1;
			}
		}
		// Перебираем каждый бит и вызываем прерывание для установленных битов с наименьшим и наивысшим приоритетом
		for (int i = 0; i < 5; i++)
		{
			if (memory.IF.is_bit_set(i))
			{
				if (memory.IE.is_bit_set(i))
				{
					// IME только отключает обслуживание прерываний,
					// а не всю функциональность прерывания
					if (cpu.interrupt_master_enable)
					{
						service_interrupt(i);
					}
				}
			}
		}
	}
}

void Emulator::service_interrupt(Byte id)
{
	cpu.interrupt_master_enable = false;
	memory.IF.clear_bit(id);

	// Помещаем текущий адрес выполнения в стек
	memory.write(--cpu.reg_SP, high_byte(cpu.reg_PC));
	memory.write(--cpu.reg_SP, low_byte(cpu.reg_PC));

	switch (id)
	{
	case INTERRUPT_VBLANK: cpu.reg_PC = 0x40; break;
	case INTERRUPT_LCDC:   cpu.reg_PC = 0x48; break;
	case INTERRUPT_TIMER:  cpu.reg_PC = 0x50; break;
	case INTERRUPT_SERIAL: cpu.reg_PC = 0x58; break;
	case INTERRUPT_JOYPAD: cpu.reg_PC = 0x60; break;
	}
}

void Emulator::set_lcd_status()
{
	Byte status = memory.STAT.get();

	Byte current_line = memory.LY.get();
	// извлечь текущий режим LCD
	Byte current_mode = status & 0x03;

	Byte mode = 0;
	bool do_interrupt = false;

	// в VBLANK устанавливаем режим 1
	if (current_line >= 144)
	{
		mode = 1; // В периоде вертикальной развёртки
		// 1 в двоичном виде
		status = set_bit(status, BIT_0);
		status = clear_bit(status, BIT_1);
		do_interrupt = is_bit_set(status, BIT_4);

	}
	else
	{
		int mode2_threshold = 456 - 80;
		int mode3_threshold = mode2_threshold - 172;

		if (scanline_counter >= mode2_threshold)
		{
			mode = 2; // Поиск в OAM RAM
			// 2 в двоичном виде
			status = set_bit(status, BIT_1);
			status = clear_bit(status, BIT_0);
			do_interrupt = is_bit_set(status, BIT_5);
		}
		else if (scanline_counter >= mode3_threshold)
		{
			mode = 3; // Передача данных в драйвер LCD
			// 3 в двоичном виде
			status = set_bit(status, BIT_1);
			status = set_bit(status, BIT_0);
		}
		else
		{
			mode = 0; // ЦП имеет доступ ко всей оперативной памяти дисплея

			// Если первый раз встречается H-blank, обновляем строку сканирования
			if (current_mode != mode)
			{
				// нарисовать текущую строку сканирования на экране
				if (current_line < 144 && display.scanlines_rendered <= 144)
					display.update_scanline(current_line);
			}

			// 0 в двоичном виде
			status = clear_bit(status, BIT_1);
			status = clear_bit(status, BIT_0);
			do_interrupt = is_bit_set(status, BIT_3);
		}
	}

	// Вошли в новый режим, запросить прерывание
	if (do_interrupt && (mode != current_mode))
		request_interrupt(INTERRUPT_LCDC);

	// проверить флаг совпадения, установить бит 2, если он совпадает
	if (memory.LY.get() == memory.LYC.get())
	{
		status = set_bit(status, BIT_2);

		if (is_bit_set(status, BIT_6))
			request_interrupt(INTERRUPT_LCDC);
	}
	// сбросить бит 2, если нет
	else
		status = clear_bit(status, BIT_2);

	memory.STAT.set(status);
	memory.video_mode = mode;
}

void Emulator::update_scanline(int cycles)
{
	scanline_counter -= cycles;

	set_lcd_status();

	if (memory.LY.get() > 153)
		memory.LY.clear();

	// Прошло достаточно времени для отрисовки следующей строки сканирования
	if (scanline_counter <= 0)
	{
		Byte current_scanline = memory.LY.get();

		// инкрементировать строку сканирования и сбросить счётчик
		memory.LY.set(++current_scanline);
		scanline_counter = 456;

		// Вошли в период VBLANK
		if (current_scanline == 144)
		{
			request_interrupt(INTERRUPT_VBLANK);
			if (display.scanlines_rendered <= 144)
				display.render();
		}
		// Сбросить счётчик, если превышено максимальное значение
		else if (current_scanline > 153)
			memory.LY.clear();
	}
}

void Emulator::save_state(int id)
{
	ofstream file;
	string filename = "./saves/" + memory.rom_name + "_" + to_string(id) + ".sav";
	file.open(filename, ios::binary | ios::trunc);

	if (!file.bad())
	{
		cpu.save_state(file);
		memory.save_state(file);
		file.close();

		cout << "записано состояние сохранения " << id << endl;
	}
}

void Emulator::load_state(int id)
{
	string filename = "./saves/" + memory.rom_name + "_" + to_string(id) + ".sav";
	ifstream file(filename, ios::binary);

	if (file.is_open())
	{
		cpu.load_state(file);
		memory.load_state(file);
		file.close();

		cout << "загружено состояние " << id << endl;
	}
}
