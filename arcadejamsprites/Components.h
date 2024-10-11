#pragma once
#include "pch.h"
using namespace DirectX::SimpleMath;
#include <unordered_map>


// not yet in use, should be in a different branch
struct Entity {
	__int32 id;
};

struct AABB {
	int width;
	int height;
	int depth;
};

class System {
public:
	System();
};

class Position : public System {
public:

	Vector3 Get(__int32 entityId) {
		return entities.at(entityId);
	}

	void Add(Entity& e) {
		entityIds.push_back(e.id);
	}

	void Add(const Entity e, const Vector3 pos) {
		entityIds.push_back(e.id);
		entities.insert({ e.id, pos });
	}

private:
	std::vector<__int32> entityIds;
	std::unordered_map<__int32, Vector3> entities;
};

class Projectile : public System {
public:
	void Update() {

	}
};

struct Entities {
	std::unordered_map<__int32, Entity> entities;

	Entity getEntity(__int32 id) {
		return entities.at(id);
	}
};