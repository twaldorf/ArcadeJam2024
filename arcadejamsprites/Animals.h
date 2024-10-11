#pragma once

#include "pch.h"
#include "Descriptors.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

constexpr int DOG_HP = 3;

class Animal {
public:
	boolean alive;
	Vector2 pos;
	Descriptors type = Crab;
	RECT rect = { 0, 0, 32, 32 };
	
	Animal() {
		alive = true;
	}

	Animal(Vector2 pos2) {
		alive = true;
		pos = pos2;
	}

	void update() {
		if (alive == false) {
			delete this;
		}

		pos.y += static_cast<float>(cos(pos.x) * 2.f);
	}

	void smush() {
		alive = false;
	}
};


class Octoc : public Animal {
public:
	Octoc() {
		type = Octo;
		pos = Vector2(-499.f, 0.f);
	}

	void update(const float& tt, Vector2 const projPos) {
		pos.y = projPos.y;
	}

};

class Dog : public Animal {
public:
	int hp = DOG_HP;
	Vector2 velocity;

	Dog() {
	}

	void dmg(float time) {
		hp--;
		if (hp < 1) {
			this->alive = false;
		}
	}

	void update(float& tt) {
		this->pos += velocity * tt;
	}

	void Up() {
		this->rect = { 0, 96, 32, 128 };
	}
	void Down() {
		this->rect = { 0, 0, 32, 32 };
	}
	void Left() {
		this->rect = { 0, 32, 32, 64 };
	}
	void Right() {
		this->rect = { 0, 64, 32, 96 };
	}

	void restart() {
		hp = DOG_HP;
		alive = true;
	}
};
