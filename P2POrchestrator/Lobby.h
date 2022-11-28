#pragma once
#include <vector>
#include "Task.h"
#define LOBBY_PLAYER_COUNT 2
// A lobby represents a match in a way. It holds players that are needed for a match
// Master will check its lobby queue to see if one exists, if not it will create one
class Lobby {
public:
	Lobby() : players(LOBBY_PLAYER_COUNT)
	{}
	void AddPlayer(const Task& tsk) {
		players.emplace_back(tsk);
	}
	// const Task& t, char platform, char* region, std::any worker
private:
	std::vector<Task> players;
};