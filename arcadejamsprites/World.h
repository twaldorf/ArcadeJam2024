#include "pch.h"
#include "Descriptors.h"
#include "Animals.h"
#include <random>
#include "Components.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

constexpr int CHUNK_WIDTH = 10;
constexpr int CHUNK_HEIGHT = 10;
constexpr int TILE_SCALE = 32 * 4;
constexpr int SPEED = 2;

constexpr float RESISTANCE_C = 0.9f;


class Rand_int {
public:
    Rand_int(int low, int high) :dist{ low, high } {}
    int operator()() { return dist(re); }
    void seed(int s) { re.seed(s); }
private:
    std::default_random_engine re;
    std::uniform_int_distribution<> dist;
};

class World {
public:

    struct Tile {
        Descriptors desc = Sand;
        Vector2 pos = Vector2(0.f, 0.f);
        RECT rect = RECT{ 0,0,0,0};

        boolean gapCheckVs(const Vector3& newPos) const {
            int TILE_SCALE = 32 * 2;
            if (pos.x < newPos.x + TILE_SCALE &&
                pos.x + TILE_SCALE > newPos.x &&
                pos.y < newPos.y + TILE_SCALE &&
                pos.y + TILE_SCALE > newPos.y
                ) {
                return true;
            }
            else {
                return false;
            }
        }
    };

    struct Projectile : Tile {
        Vector2 velocity;
        float baselineY;
        RECT rect = { 0, 0, 64, 64 };

        Projectile(Descriptors descr, Vector2 posi, Vector2 vel) {
            desc = descr;
            pos = posi;
            vel.Normalize(velocity);
            velocity *= SPEED * 100.f * 10.f;
            baselineY = posi.y;
            
        }

        void update(const float& delta) {
            this->velocity = velocity + Vector2(0.f, -9.8f * SPEED) * RESISTANCE_C;
            this->pos += velocity * delta;

            if (pos.y < baselineY) {
                
                pos.y = baselineY;
                velocity.y = -velocity.y * 0.4f;
                velocity.x = velocity.x * 0.5f;
                if (velocity.Length() < 200.f) {
                    velocity = Vector2(0.f, 0.f);
                }
            }
        }
    };

    std::vector<Tile*> tiles;
    std::vector<Projectile*> projectiles;
    std::vector<Animal*> animals;
    std::unique_ptr<Octoc> octo;

    World() {
        generateChunkAt(-1100,-500);
        newCrabs(40);
        
        octo = std::make_unique<Octoc>();
        Entities *e = new Entities;
    }

    ~World() {

        // delete tiles
        for (auto &t : this->tiles) {
            delete t;
        }

        // delete animals
        for (auto &a : this->animals) {
            delete a;
        }

        // delete ball
        deleteBall();
    }

    void newCrabs(int count = 20) {
        Rand_int rndx{ -400, 1000 };
        Rand_int rndy{ -1000, 1000 };
        rndy.seed(4235345);
        rndx.seed(1069345);
        for (int i = 0; i < count; ++i) {
            int x = rndx();
            int y = rndy();
            createAnimal(Vector2(x, y));
        }
    }

    void generateChunkAt(int nx, int ny) {
        for (int i = 0; i < 20; ++i) {
            for (int j = 0; j < 40; ++j) {
                int color = 0;
                RECT temp_rect = sand_rect;
                Descriptors descriptor = Sand;
                if (j == 20) {
                    temp_rect = shore_rect;
                    descriptor = Cliff;
                }
                else if (j < 20) {
                    temp_rect = water_rect;
                    descriptor = Water;
                }
                this->tiles.push_back(new Tile{ 
                    descriptor, Vector2(nx + j * TILE_SCALE, ny + i * TILE_SCALE), temp_rect 
                });
            }
        }
    }

    void createBall(const Vector3 pos, const Vector2 vel) {
        Projectile* ball = new Projectile(Ball, Vector2(pos.x, pos.y), vel);
        projectiles.push_back(ball);
    };

    void deleteBall() {
        for (auto p : projectiles) {
            delete p;
        }
        projectiles.clear();
    }

    void createAnimal(const Vector2 pos) {
        Animal* an = new Animal(pos);
        animals.push_back(an);
    }

    boolean checkForCollisions(const Vector3& newPos) const {
        
        for (auto& t : tiles) {
            if (t->desc == Cliff && t->gapCheckVs(newPos)) {
                return true;
            }
        }
        
        return false;
    }

    // Inclusive vector collision checker for tile based entities
    template <typename T>
    boolean checkForCollision(const T& newPos, const Vector2 pos) const {
        int TILE_SCALE = 36;
        if (pos.x < newPos.x + TILE_SCALE &&
            pos.x + TILE_SCALE > newPos.x &&
            pos.y < newPos.y + TILE_SCALE &&
            pos.y + TILE_SCALE > newPos.y
            ) {
            return true;
        }
        else {
            return false;
        }
    }

private:
    const RECT sand_rect = { 32, 32, 64, 64 };

    const RECT water_rect = { 700, 0, 732, 32 };

    const RECT shore_rect = { 876, 128, 912, 160 };

    const RECT ball_rect = { 0, 0, 16, 16 };
};
