#include "display.h"

void Display::init(Memory* _memory)
{
	memory = _memory;

	int scale = 5;

	window.create(sf::VideoMode(width, height), "ComradeTech Gameboy");
	window.setSize(sf::Vector2u(width * scale, height * scale));
	window.setKeyRepeatEnabled(false);

	bg_array.create(160, 144, sf::Color(255, 0, 255));
	window_array.create(160, 144, sf::Color(0, 0, 0, 0));
	sprites_array.create(160, 144, sf::Color(0, 0, 0, 0)); // прозрачный

	shades_of_gray[0x0] = sf::Color(255, 255, 255); // 0x0 - Белый
	shades_of_gray[0x1] = sf::Color(198, 198, 198); // 0x1 - Светло-серый
	shades_of_gray[0x2] = sf::Color(127, 127, 127); // 0x2 - Темно-серый
	shades_of_gray[0x3] = sf::Color(0, 0, 0);       // 0x3 - Черный/**/
}

void Display::render()
{
	if (!is_lcd_enabled())
		return;

	window.clear(sf::Color::Transparent);

	// очистка существующих данных спрайтов и окна
	sprites_array.create(160, 144, sf::Color::Transparent);

	bool do_sprites = memory->LCDC.is_bit_set(BIT_1);

	if (do_sprites)
		render_sprites();

	sf::Texture bg_texture;
	sf::Texture window_texture;
	sf::Texture sprites_texture;

	window_texture.loadFromImage(window_array);
	bg_texture.loadFromImage(bg_array);
	sprites_texture.loadFromImage(sprites_array);

	window_sprite.setTexture(window_texture);
	bg_sprite.setTexture(bg_texture);
	sprites_sprite.setTexture(sprites_texture);

	window.draw(bg_sprite);
	window.draw(window_sprite);
	window.draw(sprites_sprite);

	window.display();
}

void Display::clear_window()
{
	window_array.create(160, 144, sf::Color::Transparent);
}

void Display::update_scanline(Byte current_scanline)
{
	scanlines_rendered++;

	bool do_background = memory->LCDC.is_bit_set(BIT_0);
	bool do_window = memory->LCDC.is_bit_set(BIT_5);

	if (do_background)
		update_bg_scanline(current_scanline);

	if (do_window)
		update_window_scanline(current_scanline);
}

void Display::update_bg_scanline(Byte current_scanline)
{
	bool bg_code_area = memory->LCDC.is_bit_set(BIT_3);

	if (debug_enabled)
	{
		bg_code_area = force_bg_map;
	}

	Address tile_map_location = (bg_code_area) ? 0x9C00 : 0x9800;
	Byte scroll_x = memory->SCX.get();
	Byte scroll_y = memory->SCY.get();

	//cout << hex << (int)scroll_x << " y: " << (int)scroll_y << endl;

	Byte palette = memory->BGP.get();

	// Для каждого пикселя в строке 160x1:
	// 1. Вычисляем, где находится пиксель в общей карте фона 256x256, с учетом ScrollX и Y
	// 2. Получаем идентификатор тайла, на котором находится этот пиксель
	// 3. Получаем цвет пикселя на основе координат относительно сетки тайлов 8x8
	// 4. Отображаем пиксель в 160x144 виде дисплея

	int y = current_scanline;

	// Итерируем слева направо по экрану отображения (x = 0 -> 160)
	for (int x = 0; x < 160; x++)
	{
		// 1. Получаем координаты пикселя X,Y в общей карте фона, с учетом ScrollX и Y
		int map_x = (int)scroll_x + x;
		int map_y = (int)scroll_y + y;

		// Корректируем координаты карты, если они выходят за пределы области 256x256, чтобы зациклиться
		map_x = (map_x >= 256) ? map_x - 256 : map_x;
		map_y = (map_y >= 256) ? map_y - 256 : map_y;

		// 2. Получаем идентификатор тайла, на котором находится этот пиксель
		int tile_col = floor(map_x / 8);
		int tile_row = floor(map_y / 8);
		int tile_map_id = (tile_row * 32) + tile_col;
		Address loc = tile_map_location + tile_map_id;
		Byte tile_id = memory->read(loc);

		// 3. Получаем цвет пикселя на основе координат относительно сетки тайлов 8x8
		// 4. Отображаем пиксель в 160x144 виде дисплея
		int tile_x_pixel = map_x % 8;
		int tile_y_pixel = map_y % 8;

		// Инвертируем пиксели X, потому что они хранятся задом наперед
		tile_x_pixel = abs(tile_x_pixel - 7);

		update_bg_tile_pixel(palette, x, y, tile_x_pixel, tile_y_pixel, tile_id);
	}
}

void Display::update_window_scanline(Byte current_scanline)
{
	// Получаем текущую карту тайлов окна
	bool window_code_area = memory->LCDC.is_bit_set(BIT_6);

	Address tile_map_location = (window_code_area) ? 0x9C00 : 0x9800;

	int window_x = (int)memory->WX.get();
	int window_y = (int)memory->WY.get();

	Byte palette = memory->BGP.get();

	// Для каждого пикселя в строке 160x1:
	// 1. Вычисляем, где находится пиксель в общей карте тайлов 256x256, с учетом ScrollX и Y
	// 2. Получаем идентификатор тайла, на котором находится этот пиксель
	// 3. Получаем цвет пикселя на основе координат относительно сетки тайлов 8x8
	// 4. Отображаем пиксель в 160x144 виде дисплея

	int y = (int)current_scanline;

	// Итерируем слева направо по экрану отображения (x = 0 -> 160)
	for (int x = 0; x < 160; x++)
	{
		// ОКНО ОТНОСИТСЯ К ЭКРАНУ
		// Сдвигаем X и Y пиксели на основе значения регистра окна
		int display_x = x + window_x - 7;
		int display_y = y;

		// 1. Получаем идентификатор тайла, на котором находится этот пиксель
		int tile_col = floor(x / 8);
		int tile_row = floor((y - window_y) / 8);
		int tile_map_id = (tile_row * 32) + tile_col;
		Address loc = tile_map_location + tile_map_id;
		Byte tile_id = memory->read(loc);

		// 2. Получаем цвет пикселя на основе координат относительно сетки тайлов 8x8
		// 3. Отображаем пиксель в 160x144 виде дисплея
		int tile_x_pixel = x % 8;
		int tile_y_pixel = y % 8;

		// Инвертируем пиксели X, потому что они хранятся задом наперед
		tile_x_pixel = abs(tile_x_pixel - 7);

		if (current_scanline == 128)
			bool breakpoint = true;

		if (current_scanline < window_y)
		{
			window_array.setPixel(x, y, sf::Color::Transparent);
		}
		else
		{
			//window_array.setPixel(x, y, sf::Color::Blue);
			update_window_tile_pixel(palette, display_x, display_y, tile_x_pixel, tile_y_pixel, tile_id);
		}
	}
}

void Display::update_bg_tile_pixel(Byte palette, int display_x, int display_y, int tile_x, int tile_y, Byte tile_id)
{
	bool bg_char_selection = memory->LCDC.is_bit_set(BIT_4);

	if (debug_enabled)
	{
		bg_char_selection = force_bg_map;
	}

	// Определяем, где хранятся текущие данные характера фона
	// если выбрана область=0, то область фона 0x8800-0x97FF и идентификатор тайла определяется как SIGNED -128 до 127
	// 0x9000 представляет адрес нулевого идентификатора в этом диапазоне
	Address bg_data_location = (bg_char_selection) ? 0x8000 : 0x9000;
	Address offset;

	// 0x8000 - 0x8FFF беззнаковый 
	if (bg_char_selection)
	{
		offset = (tile_id * 16) + bg_data_location;
	}
	// 0x8800 - 0x97FF знаковый
	else
	{
		Byte_Signed direction = (Byte_Signed)tile_id;
		Byte_2 temp_offset = (bg_data_location)+(direction * 16);
		offset = (Address)temp_offset;
	}

	Byte
		high = memory->read(offset + (tile_y * 2) + 1),
		low = memory->read(offset + (tile_y * 2));

	sf::Color color = get_pixel_color(palette, low, high, tile_x, false);
	bg_array.setPixel(display_x, display_y, color);
}

void Display::update_window_tile_pixel(Byte palette, int display_x, int display_y, int tile_x, int tile_y, Byte tile_id)
{
	if (display_x >= 160 || display_x < 0)
		return;
	if (display_y >= 144 || display_y < 0)
		return;

	bool bg_char_selection = memory->LCDC.is_bit_set(BIT_4);

	if (debug_enabled)
	{
		bg_char_selection = force_bg_map;
	}

	// Определяем, где хранятся текущие данные характера фона
	// если выбрана область=0, то область фона 0x8800-0x97FF и идентификатор тайла определяется как SIGNED -128 до 127
	// 0x9000 представляет адрес нулевого идентификатора в этом диапазоне
	Address bg_data_location = (bg_char_selection) ? 0x8000 : 0x9000;
	Address offset;

	// 0x8000 - 0x8FFF беззнаковый 
	if (bg_char_selection)
	{
		offset = (tile_id * 16) + bg_data_location;
	}
	// 0x8800 - 0x97FF знаковый
	else
	{
		Byte_Signed direction = (Byte_Signed)tile_id;
		Byte_2 temp_offset = (bg_data_location)+(direction * 16);
		offset = (Address)temp_offset;
	}

	Byte
		high = memory->read(offset + (tile_y * 2) + 1),
		low = memory->read(offset + (tile_y * 2));

	sf::Color color = get_pixel_color(palette, low, high, tile_x, false);

	window_array.setPixel(display_x, display_y, color);
}

void Display::render_sprites()
{
	Address sprite_data_location = 0xFE00;
	Byte palette_0 = memory->OBP0.get();
	Byte palette_1 = memory->OBP1.get();

	bool use_8x16_sprites = memory->LCDC.is_bit_set(BIT_2);

	// 160 байт данных спрайтов / 4 байта на спрайт = максимум 40 спрайтов для отображения
	// Начинаем с 39 -> чтобы сохранить правильный приоритет
	for (int sprite_id = 39; sprite_id >= 0; sprite_id--)
	{
		Address offset = sprite_data_location + (sprite_id * 4);
		int y_pos = ((int)memory->read(offset)) - 16;
		int x_pos = ((int)memory->read(offset + 1)) - 8;

		Byte tile_id = memory->read(offset + 2);
		Byte flags = memory->read(offset + 3);

		bool use_palette_1 = is_bit_set(flags, BIT_4);
		Byte sprite_palette = (use_palette_1) ? palette_1 : palette_0;

		// Если включен 8x16 режим, то шаблон тайла для верхнего - VAL & 0xFE
		// Нижний 8x8 тайл - VAL | 0x1

		if (use_8x16_sprites)
		{
			tile_id = tile_id & 0xFE;
			Byte tile_id_bottom = tile_id | 0x01;
			render_sprite_tile(sprite_palette, x_pos, y_pos, tile_id, flags);
			render_sprite_tile(sprite_palette, x_pos, y_pos + 8, tile_id_bottom, flags);
		}
		else
		{
			render_sprite_tile(sprite_palette, x_pos, y_pos, tile_id, flags);
		}
	}
}

void Display::render_sprite_tile(Byte palette, int start_x, int start_y, Byte tile_id, Byte flags)
{
	Address sprite_data_location = 0x8000;

	bool priority = is_bit_set(flags, BIT_7);
	bool mirror_y = is_bit_set(flags, BIT_6);
	bool mirror_x = is_bit_set(flags, BIT_5);

	// Если установлен в ноль, то спрайт всегда отображается поверх фона
	// Если установлен в 1, спрайт скрыт за фоном и окном
	// если цвет фона или окна не белый, он все равно отображается поверх
	bool hide_sprite = is_bit_set(flags, BIT_7);

	for (int y = 0; y < 8; y++)
	{
		int offset = (tile_id * 16) + sprite_data_location;

		Byte
			high = memory->read(offset + (y * 2) + 1),
			low = memory->read(offset + (y * 2));

		for (int x = 0; x < 8; x++)
		{
			int pixel_x = (mirror_x) ? (start_x + x) : (start_x + 7 - x);
			int pixel_y = (mirror_y) ? (start_y + 7 - y) : (start_y + y);

			// предотвращаем отрисовку пикселей за пределами экрана
			sf::Vector2u bounds = sprites_array.getSize();

			if (pixel_x < 0 || pixel_x >= bounds.x)
				continue;
			if (pixel_y < 0 || pixel_y >= bounds.y)
				continue;

			sf::Color color = get_pixel_color(palette, low, high, x, true);

			// Если цвет в фоне/окне отличается от белого, скрыть пиксель спрайта
			sf::Color bg_color = bg_array.getPixel(pixel_x, pixel_y);

			if (priority)
			{
				if (bg_color != shades_of_gray[0x0])
				{
					continue;
					//color = sf::Color::Transparent;
				}
			}

			sprites_array.setPixel(pixel_x, pixel_y, color);
		}
	}
}

// Возвращает цвет пикселя в X бите на основе 2 соответствующих байтов строки
sf::Color Display::get_pixel_color(Byte palette, Byte top, Byte bottom, int bit, bool is_sprite)
{
	// Определяем, какие цвета применять к каждому коду цвета на основе данных палитры
	Byte color_3_shade = (palette >> 6);        // извлекаем биты 7 и 6
	Byte color_2_shade = (palette >> 4) & 0x03; // извлекаем биты 5 и 4
	Byte color_1_shade = (palette >> 2) & 0x03; // извлекаем биты 3 и 2
	Byte color_0_shade = (palette & 0x03);      // извлекаем биты 1 и 0

	// Получаем код цвета из двух определяющих байтов
	Byte first = (Byte)is_bit_set(top, bit);
	Byte second = (Byte)is_bit_set(bottom, bit);
	Byte color_code = (second << 1) | first;

	sf::Color result;

	switch (color_code)
	{
	case 0x0: return (is_sprite) ? sf::Color::Transparent : shades_of_gray[color_0_shade];
	case 0x1: return shades_of_gray[color_1_shade];
	case 0x2: return shades_of_gray[color_2_shade];
	case 0x3: return shades_of_gray[color_3_shade];
	default:  return sf::Color(255, 0, 255); // цвет ошибки
	}
}

bool Display::is_lcd_enabled()
{
	return memory->LCDC.is_bit_set(BIT_7);
}
