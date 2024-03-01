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

	Emulator(); // �����������
	void run(); // ������ ��������
	CPU cpu; // ����������� ���������
	Memory memory; // ������
	Display display; // �������

private:

	float framerate = 60; // ������� ������

	// -------- EVENTS ------- //
	void handle_events(); // ��������� �������

	// -------- JOYPAD ------- //
	void key_pressed(Key key); // ��������� ������� �������
	void key_released(Key key); // ��������� ���������� �������
	int get_key_id(Key key); // ��������� �������������� �������

	// -------- SAVESTATES ------- //
	void save_state(int id); // ���������� ���������
	void load_state(int id); // �������� ���������

	// --------- DIVIDER --------- //
	int divider_counter = 0; // ������� ��������
	int divider_frequency = 16384; // ������� �������� (16384 �� ��� ������ 256 ������ ��)
	void update_divider(int cycles); // ���������� ��������

	// ----------TIMERS ---------- //
	int timer_counter = 0; // ������� �������
	Byte timer_frequency = 0; // ������� �������
	void update_timers(int cycles); // ���������� ��������
	bool timer_enabled(); // �������� ���������� �������
	Byte get_timer_frequency(); // ��������� ������� �������
	void set_timer_frequency(); // ��������� ������� �������

	// ------- INTERRUPTS ------- //
	void request_interrupt(Byte id); // ������ ����������
	void do_interrupts(); // ��������� ����������
	void service_interrupt(Byte id); // ��������� ����������

	// ------ LCD Display ------ //
	int scanline_counter = 456; // ����� ������ �� ��������� ����� ������ ������������
	void set_lcd_status(); // ��������� ��������� LCD
	void update_scanline(int cycles); // ���������� ������ ������������
};
