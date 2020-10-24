﻿#pragma once
#include "charlie.hpp"
#include "sdl_collider.h"
#include "sdl_sprite.hpp"

namespace charlie
{
	struct Time;
	struct Camera;
	struct Vector2;

	struct Projectile
	{
		Projectile();
		explicit Projectile(Vector2 pos, Vector2 dir, uint32 id, uint32 owner);
		explicit Projectile(Vector2 pos, float rot, uint32 id,  uint32 owner);
		
		void update(Time deltaTime);
		void render(SDL_Rect cameraPos);
		void load_sprite(const char* body, int srcX, int srcY, int srcW, int srcH);
		void destroy();
		void on_collision();

		// SDL
		SDL_Renderer* renderer_;
		SDL_Rect window_rect_;
		SDLSprite* sprite_;
		SDL_Point point;
		RectangleCollider collider_;

		uint32 id_;
		Transform transform_;
		uint32 owner_;
		float speed_;
		Vector2 direction_;
	};
}
