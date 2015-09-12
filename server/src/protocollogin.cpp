/**
 * The Forgotten Server - a free and open-source MMORPG server emulator
 * Copyright (C) 2015  Mark Samman <mark.samman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "otpch.h"

#include "protocollogin.h"

#include "outputmessage.h"
#include "connection.h"
#include "rsa.h"
#include "tasks.h"

#include "configmanager.h"
#include "tools.h"
#include "iologindata.h"
#include "ban.h"
#include "game.h"

extern ConfigManager g_config;
extern Game g_game;

void ProtocolLogin::disconnectClient(const std::string& message, uint16_t version)
{
	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if (output) {
		output->addByte(version >= 1076 ? 0x0B : 0x0A);
		output->addString(message);
		OutputMessagePool::getInstance()->send(output);
	}

	getConnection()->close();
}

void ProtocolLogin::getCharacterList(const std::string& accountName, const std::string& password, uint16_t version)
{
	bool cast_login = false;
	if ((accountName.empty() && password.empty()) || (accountName.empty() && !password.empty())) {
		cast_login = true;
	}

	if (!cast_login && accountName.empty()) {
		disconnectClient("Invalid account name.", version);
		return;
	}

	Account account;
	if (!cast_login && !IOLoginData::loginserverAuthentication(accountName, password, account)) {
		disconnectClient("Account name or password is not correct.", version);
		return;
	}

	OutputMessage_ptr output = OutputMessagePool::getInstance()->getOutputMessage(this, false);
	if (output) {
		//Update premium days
		if (!cast_login) {
			Game::updatePremium(account);
		}

		const std::string& motd = g_config.getString(ConfigManager::MOTD);
		if (!motd.empty()) {
			//Add MOTD
			output->addByte(0x14);

			std::ostringstream ss;
			ss << g_game.getMotdNum() << "\n" << motd;
			output->addString(ss.str());
		}

		if(!cast_login) {
			//SessionKey
			output->addByte(0x28);
			output->addString(accountName + "\n" + password);
		}

		//Add char list
		output->addByte(0x64);

		if (cast_login) {
			std::vector<std::pair<uint32_t, std::string> > casts;
			cast_login = false;

			g_game.lockPlayers();
			for (const auto& it : g_game.getPlayers()) {
				const Player* player = it.second;
				if (player->cast.isCasting && (player->cast.password == "" || player->cast.password == password)) {
					casts.push_back(std::make_pair(player->getCastViewerCount(), player->getName()));
					cast_login = true;
				}
			}
			g_game.unlockPlayers();

			std::sort(casts.begin(), casts.end(),
				[](const std::pair<uint32_t, std::string>& lhs, const std::pair<uint32_t, std::string>& rhs) {
				return lhs.first > rhs.first;
			}
			);

			if(casts.size() == 0) {
				disconnectClient("Currently there are no casts available.", version);
				return;
			}

			if (cast_login) {
				output->addByte(casts.size()); // number of worlds
				int i = 0;
				for (auto it : casts) {
					int32_t count = it.first;
					output->addByte(i); // world id
					std::ostringstream os;
					os << count;
					if (count == 1) {
						os << " viewer";
					} else {
						os << " viewers";
					}

					output->addString(os.str());
					output->addString(g_config.getString(ConfigManager::IP));
					output->add<uint16_t>(g_config.getNumber(ConfigManager::GAME_PORT));
					output->addByte(0);
					i++;
				}


				output->addByte((uint8_t)casts.size());
				i = 0;
				for (auto it : casts) {
					output->addByte(i);
					output->addString(it.second);
					i++;
				}
			}
		}

		if (!cast_login) {
			output->addByte(1); // number of worlds
			output->addByte(0); // world id
			output->addString(g_config.getString(ConfigManager::SERVER_NAME));
			output->addString(g_config.getString(ConfigManager::IP));
			output->add<uint16_t>(g_config.getNumber(ConfigManager::GAME_PORT));
			output->addByte(0);

			output->addByte((uint8_t)account.characters.size());
			for (const std::string& characterName : account.characters) {
				output->addByte(0);
				output->addString(characterName);
			}
		}

		//Add premium days
		if (g_config.getBoolean(ConfigManager::FREE_PREMIUM)) {
			output->add<uint16_t>(0xFFFF);    //client displays free premium
		} else {
			output->add<uint16_t>(account.premiumDays);
		}

		OutputMessagePool::getInstance()->send(output);
	}

	getConnection()->close();
}

void ProtocolLogin::onRecvFirstMessage(NetworkMessage& msg)
{
	if (g_game.getGameState() == GAME_STATE_SHUTDOWN) {
		getConnection()->close();
		return;
	}

	msg.skipBytes(2); // client OS

	uint16_t version = msg.get<uint16_t>();
	if (version >= 971) {
		msg.skipBytes(17);
	} else {
		msg.skipBytes(12);
	}
	/*
	 * Skipped bytes:
	 * 4 bytes: protocolVersion
	 * 12 bytes: dat, spr, pic signatures (4 bytes each)
	 * 1 byte: 0
	 */

#define dispatchDisconnectClient(err) g_dispatcher.addTask(createTask(std::bind(&ProtocolLogin::disconnectClient, this, err, version)))

	if (version <= 760) {
		dispatchDisconnectClient("Only clients with protocol " CLIENT_VERSION_STR " allowed!");
		return;
	}

	if (!Protocol::RSA_decrypt(msg)) {
		getConnection()->close();
		return;
	}

	uint32_t key[4];
	key[0] = msg.get<uint32_t>();
	key[1] = msg.get<uint32_t>();
	key[2] = msg.get<uint32_t>();
	key[3] = msg.get<uint32_t>();
	enableXTEAEncryption();
	setXTEAKey(key);

	if (version < CLIENT_VERSION_MIN || version > CLIENT_VERSION_MAX) {
		dispatchDisconnectClient("Only clients with protocol " CLIENT_VERSION_STR " allowed!");
		return;
	}

	if (g_game.getGameState() == GAME_STATE_STARTUP) {
		dispatchDisconnectClient("Gameworld is starting up. Please wait.");
		return;
	}

	if (g_game.getGameState() == GAME_STATE_MAINTAIN) {
		dispatchDisconnectClient("Gameworld is under maintenance.\nPlease re-connect in a while.");
		return;
	}

	BanInfo banInfo;
	if (IOBan::isIpBanned(getConnection()->getIP(), banInfo)) {
		if (banInfo.reason.empty()) {
			banInfo.reason = "(none)";
		}

		std::ostringstream ss;
		ss << "Your IP has been banned until " << formatDateShort(banInfo.expiresAt) << " by " << banInfo.bannedBy << ".\n\nReason specified:\n" << banInfo.reason;
		dispatchDisconnectClient(ss.str());
		return;
	}

#undef dispatchDisconnectClient
	std::string accountName = msg.getString();
	std::string password = msg.getString();
	g_dispatcher.addTask(createTask(std::bind(&ProtocolLogin::getCharacterList, this, accountName, password, version)));
}
