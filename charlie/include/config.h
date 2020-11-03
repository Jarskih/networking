﻿#pragma once

#ifndef CONFIG_HPP_INCLUDED
#define CONFIG_HPP_INCLUDED

namespace charlie
{
	namespace config
	{
		static const int  PORT = 54345;
		static const uint8  IP_A = 82;
		static const uint8  IP_B = 117;
		static const uint8  IP_C = 111;
		static const uint8  IP_D = 100;
		static const int  SCREEN_WIDTH = 640;
		static const int  SCREEN_HEIGHT = 480;
		static const int  PLAYER_WIDTH = 150;
		static const int  PLAYER_HEIGHT = 150;
		static const float PLAYER_SPEED = 100.0f;
		static const float PLAYER_TURN_SPEED = 50.0f;
		static const int  PROJECTILE_WIDTH = 25;
		static const int  PROJECTILE_HEIGHT = 25;
		static const double  FIRE_DELAY = 2.0;
		static const float PROJECTILE_SPEED = 600.0f;
		static const Time PROJECTILE_LIFETIME = Time(3.0);
		static const char* TANK_BODY_SPRITE = "../assets/tank_body.png";
		static const char* TANK_TURRET_SPRITE = "../assets/tank_turret.png";
		static const char* TANK_SHELL = "../assets/Light_Shell.png";
		static const char* LEVEL1 = "../assets/map.txt";
		static const char* FONT_PATH = "../assets/font/font.ttf";
		static const int LEVEL_OBJECT_WIDTH = 50;
		static const int LEVEL_OBJECT_HEIGHT = 50;
		static const char* MENU_SCREEN = "../assets/menu_screen.png";
	};
}
#endif
