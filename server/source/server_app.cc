// server_app.cc

#include "server_app.hpp"
#include <charlie_messages.hpp>
#include <cstdio>
#include <cmath>

#include "config.h"
#include "reliable_events.h"

ServerApp::ServerApp()
	: tickrate_(1.0 / 60.0)
	  , tick_(0)
	  , index_(0)
	  , projectile_index_(0)
{
}

bool ServerApp::on_init()
{
	network_.set_send_rate(Time(1.0 / 20.0));
	network_.set_allow_connections(true);
	if (!network_.initialize(network::IPAddress(network::IPAddress::ANY_HOST, 54345))) {
		return false;
	}

	network_.add_service_listener(this);

	auto data = Leveldata();
	data.create_level(config::LEVEL1);
	level_manager_ = LevelManager();
	level_manager_.load_assets(data);

	level_width_ = level_manager_.width_;
	level_heigth_ = level_manager_.height_;

	return true;
}

void ServerApp::on_exit()
{
}

bool ServerApp::on_tick(const Time& dt)
{
	input_handler_.HandleEvents();
	if (input_handler_.IsKeyDown(SDL_SCANCODE_ESCAPE))
	{
		return false;
	}

	accumulator_ += dt;
	while (accumulator_ >= tickrate_) {
		accumulator_ -= tickrate_;
		tick_++;
		
		read_input_queue();
		update_players(dt);
		
		for (auto& projectile : projectiles_)
		{
			projectile.update(dt);
		}

		check_collisions();

		for (auto& id : players_to_remove_)
		{
			remove_player(id);
		}

		for (auto& id : projectiles_to_remove_)
		{
			remove_projectile(id);
		}
	}
	
	return true;
}

void ServerApp::on_draw()
{
	char myString[10] = "";
	sprintf_s(myString, "%ld", long(tick_));

	for (auto& player : players_)
	{
		player.render(cam_);
	}

	for (auto& projectile : projectiles_)
	{
		projectile.render(cam_);
	}
}

void ServerApp::on_timeout(network::Connection* connection)
{
	connection->set_listener(nullptr);
	const uint32 id = clients_.find_client((uint64)connection);

	players_to_remove_.push_back(id);
	create_destroy_event(id, EventType::DESTROY_PLAYER);
		
	clients_.remove_client((uint64)connection);
	printf("Player %i disconnected. Players %i \n", id, (int)clients_.clients_.size());
}

void ServerApp::on_connect(network::Connection* connection)
{
	connection->set_listener(this);

	const auto id = clients_.add_client((uint64)connection);
	
	Player player;
	player.id_ = id;

	Vector2 pos = Vector2(20.0f + random_() % 200, 200.0f + random_() % 100);
	player.init(renderer_.get_renderer(), pos, index_);
	player.load_body_sprite(config::TANK_BODY_SPRITE, 0, 0, config::PLAYER_WIDTH, config::PLAYER_HEIGHT);
	player.load_turret_sprite(config::TANK_TURRET_SPRITE, 0, 0, config::PLAYER_WIDTH, config::PLAYER_HEIGHT);

	for (const Player& p : players_)
	{
		create_spawn_event(player.id_, p, EventType::SPAWN_PLAYER);
		create_spawn_event(p.id_, player, EventType::SPAWN_PLAYER);
	}
	
	players_.push_back(player);
	index_ += 1;

	printf("player joined. players: %i\n", (int)clients_.clients_.size());
}

void ServerApp::on_disconnect(network::Connection* connection)
{
	connection->set_listener(nullptr);

	const uint32 id = clients_.find_client((uint64)connection);
	
	players_to_remove_.push_back(id);

	create_destroy_event(id, EventType::DESTROY_PLAYER);

	clients_.remove_client((uint64)connection);

	printf("player disconnected. players: %i\n", (int)clients_.clients_.size());
}

void ServerApp::on_acknowledge(network::Connection* connection,
	const uint16 sequence)
{
}

void ServerApp::on_receive(network::Connection* connection,
	network::NetworkStreamReader& reader)
{
	const uint32 id = clients_.find_client((uint64)connection);

	while (reader.position() < reader.length()) {
		switch (reader.peek()) {
		case(network::NETWORK_MESSAGE_INPUT_COMMAND):
			{
				network::NetworkMessageInputCommand command;
				if (!command.read(reader)) {
					assert(!"could not read command!");
				}

				for (auto& player : players_) {
					if (player.id_ == id) {
						gameplay::InputCommand cmd{};
						cmd.id_ = id;
						cmd.input_bits_ = command.bits_;
						cmd.rot_ = command.rot_;
						cmd.fire_ = command.fire_;
						input_queue_.push(cmd);
						break;
					}
				}
			} break;
		case(network::NETWORK_MESSAGE_ACK):
			{
				network::NetworkMessageAck msg;
				if (!msg.read(reader)) {
					assert(!"could not read command!");
				}

				const auto message = reliable_queue_.get_message(connection->acknowledge_);
				if(message.seq_ != 0)
				{
					// printf("Spawn command %i acknowledged on sequence %i \n", msg.message_id_, message.seq_);
					remove_from_array(spawn_event_list, message.id_);
				}
			} break;
		default: 
			{
				printf("Unknown message %i", (int)reader.peek());
				assert(!"unknown message type received from client!");
			} break;
		}
	}
}

void ServerApp::on_send(network::Connection* connection,
	const uint16 sequence,
	network::NetworkStreamWriter& writer)
{
	const uint32 id = clients_.find_client((uint64)connection);

	{
		network::NetworkMessageServerTick message(Time::now().as_ticks(), tick_);
		if (!message.write(writer)) {
			assert(!"failed to write message!");
		}
	}

	// Send reliable messages
	{
		for (Event reliable_event : reliable_events_)
		{
			if (id == reliable_event.send_to_)
			{
				gameplay::Message msg;
				msg.id_ = reliable_event.id_;
				msg.seq_ = sequence;
					
				// Keep track of sent reliable messages
				reliable_queue_.add_message(tick_, msg);

				write_message(reliable_event, writer);
				printf("Sent reliable message to %i \n", id);
			}
		}
	}

	{
		// Send player update and entity updates to one player
		for (auto& player : players_)
		{
			if (player.id_ == id)
			{
				network::NetworkMessagePlayerState message(player.transform_, player.turret_rotation_);
				if (!message.write(writer)) {
					assert(!"failed to write message!");
				}
				continue;
			}

			network::NetworkMessageEntityState message(player.transform_, player.turret_rotation_, player.id_);
			if (!message.write(writer)) {
				assert(!"failed to write message!");
			}
		}
	}
}

void ServerApp::read_input_queue()
{
	while (!input_queue_.empty())
	{
			const gameplay::InputCommand cmd = input_queue_.front();
			input_queue_.pop();

			for (auto& player : players_)
			{
				if(player.id_ == cmd.id_)
				{
					player.input_bits_ = cmd.input_bits_;
					player.turret_rotation_ = cmd.rot_;
					player.fire_ = cmd.fire_;
					break;
				}	
			}
	}
}

void ServerApp::update_players(const Time& dt)
{
			for (auto& player : players_)
		{
			float direction = 0;
			float rotation = 0;

			const bool player_move_up = player.get_input_bits() & (1 << int32(gameplay::Action::Up));
			const bool player_move_down = player.get_input_bits() & (1 << int32(gameplay::Action::Down));
			const bool player_move_left = player.get_input_bits() & (1 << int32(gameplay::Action::Left));
			const bool player_move_right = player.get_input_bits() & (1 << int32(gameplay::Action::Right));

			if (player_move_up) {
				direction -= 1.0f;
			}
			if (player_move_down) {
				direction += 1.0f;
			}
			if (player_move_left) {
				rotation -= 1.0f;
			}
			if (player_move_right) {
				rotation += 1.0f;
			}

			if (abs(rotation) > 0.0f)
			{
				const float rot = player.transform_.rotation_ + rotation * player.tank_turn_speed_ * dt.as_seconds();
				player.transform_.set_rotation(rot);
			}

			if (abs(direction) > 0.0f) {
				player.transform_.position_ += player.transform_.forward() * direction * player.speed_ * dt.as_seconds();
			}


			if ((player.transform_.position_.x_ < 0) || (player.transform_.position_.x_ + (float)player.body_sprite_->get_area().w > level_width_))
			{
				player.transform_.position_ -= player.transform_.forward() * direction * player.speed_ * dt.as_seconds();
			}

			if ((player.transform_.position_.y_ < 0) || (player.transform_.position_.y_ + (float)player.body_sprite_->get_area().h > level_heigth_))
			{
				player.transform_.position_ -= player.transform_.forward() * direction * player.speed_ * dt.as_seconds();
			}

			{
				cam_.x = (int)player.transform_.position_.x_ + player.body_sprite_->get_area().w / 2 - config::SCREEN_WIDTH / 2;
				cam_.y = (int)player.transform_.position_.y_ + player.body_sprite_->get_area().h / 2 - config::SCREEN_HEIGHT / 2;

				if (cam_.x < 0)
				{
					cam_.x = 0;
				}
				if (cam_.y < 0)
				{
					cam_.y = 0;
				}
				if (cam_.x > level_width_ - cam_.w)
				{
					cam_.x = level_width_ - cam_.w;
				}
				if (cam_.y > level_heigth_ - cam_.h)
				{
					cam_.y = level_heigth_ - cam_.h;
				}
			}

			player.fire_acc_ += dt;
			if(player.fire_ && player.can_shoot())
			{
				spawn_projectile(player.get_shoot_pos(), player.turret_rotation_, player.id_);
				player.fire();
				create_spawn_event(projectile_index_, player, EventType::SPAWN_PROJECTILE);
			}
		}
}

void ServerApp::check_collisions()
{
	for (int i = 0 ; i < (int)players_.size(); i++)
	{
		for (int j = 0 ; j < (int)projectiles_.size(); j++)
		{
			if(CollisionHandler::IsColliding(players_[i].collider_, projectiles_[j].collider_))
			{
				players_[i].on_collision();
				projectiles_[j].on_collision();
				projectiles_to_remove_.push_back(projectiles_[j].id_);
				create_destroy_event(projectiles_[j].id_, EventType::DESTROY_PROJECTILE);
			}
		}
	}
}

void ServerApp::remove_player(const uint32 id)
{
	auto it = players_.begin();
	while (it != players_.end())
	{
		if ((*it).id_ == id)
		{
			(*it).destroy();
			players_.erase(it);
			break;
		}
		++it;
	}
}

void ServerApp::spawn_projectile(const Vector2 pos, const float rotation, const uint32 id)
{
	Projectile e(pos, rotation, projectile_index_, id);
	e.renderer_ = renderer_.get_renderer();
	e.load_sprite(config::TANK_SHELL, 0, 0, config::PROJECTILE_WIDTH, config::PROJECTILE_HEIGHT);
	projectiles_.push_back(e);
	projectile_index_ += 1;
	// printf("Spawned projectile \n");
}

void ServerApp::remove_projectile(uint32 id)
{
	auto it = projectiles_.begin();
	while (it != projectiles_.end())
	{
		if ((*it).id_ == id)
		{
			(*it).destroy();
			projectiles_.erase(it);
			break;
		}
		++it;
	}
}

void ServerApp::create_spawn_event(const uint32 owner, const Player& p, const EventType event)
{
	for (auto& player : players_)
	{
		if(player.id_ == owner)
		{
			continue;
		}

		switch(event)
		{
		case EventType::SPAWN_PLAYER:
			{
				PlayerSpawned e(owner, player.id_, p.transform_.position_);
				reliable_events_.push_back(e);
				printf("created player spawned event \n");
			} break;
		case EventType::SPAWN_PROJECTILE:
			{
				ProjectileSpawned e(projectile_index_, player.id_, owner, player.get_shoot_pos(), p.turret_rotation_);
				reliable_events_.push_back(e);
				printf("created projectile spawned event \n");
			} break;
		default:
			break;
		}
	}
	printf("reliable events in queue %i \n", (int)reliable_events_.size());
}

void ServerApp::create_destroy_event(const uint32 id, const EventType event)
{
	for (auto& player : players_)
	{
		switch(event)
		{
		case EventType::DESTROY_PLAYER:
			{
				PlayerDestroyed e(id, player.id_);
				reliable_events_.push_back(e);
				printf("created player destroy event \n");
			} break;
		case EventType::DESTROY_PROJECTILE:
			{
				ProjectileDestroyed e(id, player.id_);
				reliable_events_.push_back(e);
				printf("created projectile destroy event \n");
			} break;
		default:
			break;
		}
	}
	printf("reliable events in queue %i \n", (int)reliable_events_.size());
}

void ServerApp::write_message(const Event& reliable_event, network::NetworkStreamWriter& writer)
{
	switch(reliable_event.type_)
			{
			case(EventType::SPAWN_PLAYER):
			{
				network::NetworkMessagePlayerSpawn message(reliable_event.pos_, reliable_event.id_);
				if(!message.write(writer))
				{
					assert(!"failed to write message!");
				}	
			} break;
			case(EventType::SPAWN_PROJECTILE):
			{
				network::NetworkMessageProjectileSpawn message(reliable_event.id_, reliable_event.owner_,reliable_event.pos_, reliable_event.rot_);
				if(!message.write(writer))
				{
					assert(!"failed to write message!");
				}
			} break;
			case(EventType::DESTROY_PLAYER):
			{
				network::NetworkMessagePlayerDisconnected message(reliable_event.id_);
				if(!message.write(writer))
				{
					assert(!"failed to write message!");
				}
			} break;
			case(EventType::DESTROY_PROJECTILE):
			{
				network::NetworkMessageProjectileDestroy message(reliable_event.id_);
				if(!message.write(writer))
				{
					assert(!"failed to write message!");
				}
			} break;
			default:
				break;
			}
}

bool ServerApp::contains(const DynamicArray<uint32>& arr, const uint32 id)
{
	return std::find(arr.begin(), arr.end(), id) != arr.end();
}

void ServerApp::remove_from_array(DynamicArray<Event>& arr, uint32 id)
{
	auto it = arr.begin();
	while (it != arr.end())
	{
		if ((*it).id_ == id)
		{
			arr.erase(it);
			break;
		}
		++it;
	}
}
