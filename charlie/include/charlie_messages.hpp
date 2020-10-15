// charlie_messages.hpp

#ifndef CHARLIE_MESSAGES_HPP_INCLUDED
#define CHARLIE_MESSAGES_HPP_INCLUDED

#include <charlie.hpp>

#include "charlie_gameplay.hpp"

namespace charlie {
	namespace network {
		struct NetworkStreamReader;
		struct NetworkStreamWriter;

		enum NetworkMessageType {
			NETWORK_MESSAGE_SERVER_TICK,
			NETWORK_MESSAGE_ENTITY_STATE,
			NETWORK_MESSAGE_INPUT_COMMAND,
			NETWORK_MESSAGE_PLAYER_STATE,
			NETWORK_MESSAGE_PLAYER_SPAWN,
			NETWORK_MESSAGE_COUNT,
		};

		static_assert(NETWORK_MESSAGE_COUNT <= 255, "network message type cannot exceed 255!");

		struct NetworkMessageServerTick {
			NetworkMessageServerTick();
			explicit NetworkMessageServerTick(const int64  server_time,
				const uint32 server_tick);

			bool read(NetworkStreamReader& reader);
			bool write(NetworkStreamWriter& writer);

			template <typename Stream>
			bool serialize(Stream& stream)
			{
				bool result = true;
				result &= stream.serialize(type_);
				result &= stream.serialize(server_time_);
				result &= stream.serialize(server_tick_);
				return result;
			}

			uint8 type_;
			int64 server_time_;
			uint32 server_tick_;
		};

		struct NetworkMessageEntityState {
			NetworkMessageEntityState();
			explicit NetworkMessageEntityState(const Transform& transform, uint32 id);

			bool read(NetworkStreamReader& reader);
			bool write(NetworkStreamWriter& writer);

			template <typename Stream>
			bool serialize(Stream& stream)
			{
				bool result = true;
				result &= stream.serialize(type_);
				result &= stream.serialize(position_.x_);
				result &= stream.serialize(position_.y_);
				result &= stream.serialize(rotation_);
				result &= stream.serialize(id_);
				return result;
			}

			uint8 type_;
			Vector2 position_;
			float rotation_;
			uint32 id_;
		};

		struct NetworkMessageInputCommand {
			NetworkMessageInputCommand();
			explicit NetworkMessageInputCommand(uint8 bits);

			bool read(NetworkStreamReader& reader);
			bool write(NetworkStreamWriter& writer);

			template <typename Stream>
			bool serialize(Stream& stream)
			{
				bool result = true;
				result &= stream.serialize(type_);
				result &= stream.serialize(bits_);
				return result;
			}

			uint8 type_;
			uint8 bits_;
		};

		struct NetworkMessagePlayerState {
			NetworkMessagePlayerState();
			explicit NetworkMessagePlayerState(const Transform& transform);

			bool read(NetworkStreamReader& reader);
			bool write(NetworkStreamWriter& writer);

			template <typename Stream>
			bool serialize(Stream& stream)
			{
				bool result = true;
				result &= stream.serialize(type_);
				result &= stream.serialize(position_.x_);
				result &= stream.serialize(position_.y_);
				result &= stream.serialize(rotation_);
				return result;
			}

			uint8 type_;
			float rotation_;
			Vector2 position_;
		};

		struct NetworkMessagePlayerSpawn
		{
			NetworkMessagePlayerSpawn();
			explicit NetworkMessagePlayerSpawn(const Vector2& position, const uint32 id_);
			bool read(NetworkStreamReader& reader);
			bool write(NetworkStreamWriter& writer);

			template <typename Stream>
			bool serialize(Stream& stream)
			{
				bool result = true;
				result &= stream.serialize(type_);
				result &= stream.serialize(position_.x_);
				result &= stream.serialize(position_.y_);
				result &= stream.serialize(id_);
				return result;
			}

			uint8 type_;
			Vector2 position_;
			uint32 id_;
		};
	} // !network
} // !charlie

#endif // !CHARLIE_MESSAGES_HPP_INCLUDED
