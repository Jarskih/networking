#pragma once
#include "sdl_collider.h"
#include "sdl_sprite.hpp"
#include "sdl_window.hpp"

namespace charlie
{
	struct Camera;
	struct Vector2;

	struct Player {
		Player();
		void init(SDL_Renderer* renderer, Vector2& pos, uint32 id);
		void update(Time deltaTime, int level_height, int level_width);
		void render(SDL_Rect cameraPos);
		void load_body_sprite(const char* body, int srcX, int srcY, int srcW, int srcH);
		void load_turret_sprite(const char* turret, int srcX, int srcY, int srcW, int srcH);
		Vector2 get_shoot_pos();
		void destroy();
		uint8 get_input_bits() const;
		bool can_shoot();
		void fire();
		void on_collision();

		// SDL
		SDL_Renderer* renderer_;
		SDLSprite* body_sprite_;
		SDL_Rect body_window_rect_;
		SDLSprite* turret_sprite_;
		SDL_Rect turret_window_rect_;
		SDL_Point point;

		Transform transform_;
		RectangleCollider collider_;
		float turret_rotation_;
		uint8 input_bits_;
		uint32 id_;
		float speed_;
		float tank_turn_speed_;
		float turret_turn_speed_;
		
		bool fire_;
		Time fire_acc_;
		Time fire_delay_;

	};
}
