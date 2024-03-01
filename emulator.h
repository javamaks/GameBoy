#pragma once

#include <fstream>

#include <SFML\System.hpp>
#include <SFML\Audio.hpp>
#include "cpu.h"
#include "memory.h"
#include "display.h"

typedef sf::Keyboard::Key Key;

class Emulator
{
public:

	Emulator(); // Конструктор
	void run(); // Запуск эмуляции
	CPU cpu; // Центральный процессор
	Memory memory; // Память
	Display display; // Дисплей

private:

	float framerate = 60; // Частота кадров

	// -------- EVENTS ------- //
	void handle_events(); // Обработка событий

	// -------- JOYPAD ------- //
	void key_pressed(Key key); // Обработка нажатия клавиши
	void key_released(Key key); // Обработка отпускания клавиши
	int get_key_id(Key key); // Получение идентификатора клавиши

	// -------- SAVESTATES ------- //
	void save_state(int id); // Сохранение состояния
	void load_state(int id); // Загрузка состояния

	// --------- DIVIDER --------- //
	int divider_counter = 0; // Счётчик делителя
	int divider_frequency = 16384; // Частота делителя (16384 Гц или каждые 256 тактов ЦП)
	void update_divider(int cycles); // Обновление делителя

	// ----------TIMERS ---------- //
	int timer_counter = 0; // Счётчик таймера
	Byte timer_frequency = 0; // Частота таймера
	void update_timers(int cycles); // Обновление таймеров
	bool timer_enabled(); // Проверка активности таймера
	Byte get_timer_frequency(); // Получение частоты таймера
	void set_timer_frequency(); // Установка частоты таймера

	// ------- INTERRUPTS ------- //
	void request_interrupt(Byte id); // Запрос прерывания
	void do_interrupts(); // Обработка прерываний
	void service_interrupt(Byte id); // Обработка прерывания

	// ------ LCD Display ------ //
	int scanline_counter = 456; // Число тактов на отрисовку одной строки сканирования
	void set_lcd_status(); // Установка состояния LCD
	void update_scanline(int cycles); // Обновление строки сканирования
};
