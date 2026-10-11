#pragma once

// One run through the world from a save: the hero (GameLib's platformer
// controller with the abilities found so far, a slash with frame data, a
// limited-ammo shot, a ground pound, swimming, knockback and invincibility),
// the current room and the scroll into the next one, doors, cracked floors
// and walls, items, save and map stations, the enemies of the room (they
// come back whenever the room is entered again), the guardians and the
// escape from the core. Deterministic: the same Progress and per-tick
// Input give the same run, so the demo and recordings replay exactly. No
// window, audio or clock.
//
// What outlives a death or a quit is Progress: the world flags (doors
// opened, walls broken, items taken, guardians beaten), the map's fog, the
// abilities, keys and upgrades, and the save station. Death returns the
// hero to the last save station with the world as it is.

#include "rules.hpp"
#include "world.hpp"

#include <gamelib/core/combat.hpp>
#include <gamelib/core/crumble_tiles.hpp>
#include <gamelib/core/enemy_motion.hpp>
#include <gamelib/core/event_queue.hpp>
#include <gamelib/core/map_reveal.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/core/platformer.hpp>
#include <gamelib/core/projectiles.hpp>
#include <gamelib/core/rng.hpp>
#include <gamelib/core/skills.hpp>
#include <gamelib/core/state_machine.hpp>
#include <gamelib/core/telegraphs.hpp>
#include <gamelib/core/tile_map.hpp>
#include <gamelib/core/world_flags.hpp>
#include <gamelib/core/world_graph.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace cavern::game {

// What a save keeps.
struct Progress {
    gamelib::core::WorldFlags flags;
    gamelib::core::MapReveal reveal{1, 1};
    Abilities abilities;
    std::uint8_t keys = 0;      // bit = key 0..3
    int healthUpgrades = 0;
    int ammoUpgrades = 0;
    int saveStation = -1;       // index into World::stations(); -1: the start
    std::int64_t ticks = 0;     // play time
    int deaths = 0;
    bool finished = false;      // escaped once

    [[nodiscard]] static Progress fresh();
    [[nodiscard]] bool hasKey(const int key) const { return key >= 0 && (keys & (1u << key)) != 0; }
    [[nodiscard]] int maxHealth() const { return kStartHealth + healthUpgrades * kHealthPerUpgrade; }
    [[nodiscard]] int maxAmmo() const { return kStartAmmo + ammoUpgrades * kAmmoPerUpgrade; }
    // Items taken out of all the world's items, 0..100.
    [[nodiscard]] int completion() const;
    [[nodiscard]] int itemsTaken() const;
};

enum class Phase {
    Playing,
    Scroll,     // the camera scrolls to the next room; everything else holds
    Hazard,     // thorns: dark for a moment, then back on the last safe floor
    AbilityGet, // the scene after picking up an ability
    BossIntro,  // the guardian wakes, the exits seal, its bar fills
    BossDown,   // the guardian bursts apart
    Dying,
    Dead,       // over: the caller reloads from the save (respawn())
    Escaped,    // the end: out of the core
};

struct ShotData {
    bool enemy = false;
    int look = 0; // hero: 0; enemies: 1 bolt, 2 spore, 3 bubble, 4 beam, 5 bomb, 6 magma, 7 shard, 8 boss orb
};
using Shots = gamelib::core::Projectiles<ShotData>;
using Shot = gamelib::core::Projectile<ShotData>;

struct Enemy {
    EnemyKind kind = EnemyKind::Crawler;
    gamelib::core::WalkerBody body;
    gamelib::core::Rect previous;
    gamelib::core::Vec2 origin;
    int hp = 1;
    int maxHp = 1;
    int timer = 0;
    int state = 0;
    int age = 0;
    int flash = 0;
    int seed = 0;
    gamelib::core::Vulnerable vulnerable;
};

enum class DropKind : std::uint8_t { Health, Ammo };

struct Drop {
    DropKind kind = DropKind::Health;
    gamelib::core::WalkerBody body;
    gamelib::core::Rect previous;
    int life = 0;
};

enum class BossState : std::uint8_t { Asleep, Intro, Idle, Move, Attack, Attack2, Attack3, Recover, Dead };

struct Boss {
    BossKind kind = BossKind::Sporemother;
    gamelib::core::WalkerBody body;
    gamelib::core::Rect previous;
    gamelib::core::Vec2 home;
    int def = 0;   // index into World::bosses()
    int hp = 1;
    int maxHp = 1;
    int shown = 0; // the health bar while it fills
    int flash = 0;
    int pattern = -1;
    int count = 0;
    int tell = 0;  // 0..255: how close an attack is (the drawing winds up)
    bool flying = false;
    gamelib::core::Vec2 target;
    gamelib::core::StateMachine<BossState> sm{BossState::Asleep};
    gamelib::core::PhaseByHealth phases{0.6f, 0.3f};
    gamelib::core::Vulnerable vulnerable;
};

struct Hero {
    gamelib::core::PlatformerBody body;
    gamelib::core::Rect previous;
    gamelib::core::Vulnerable vulnerable;
    gamelib::core::Resource health{kStartHealth, kStartHealth};
    gamelib::core::Resource ammo{kStartAmmo, kStartAmmo};
    gamelib::core::Attacker slash{0, 1'000'000};
    int slashAim = 0;            // the swing in progress: -1 up, 0 forward, 1 down
    int struckId = 0;            // the swing that already struck the tiles
    int keptAim = 0;             // the aim of a press kept for the next swing
    gamelib::core::GroundPound pound;
    int shootCooldown = 0;
    int shootPose = 0;
    int hurtTicks = 0;
    bool swimming = false;
    float walked = 0.0f;
    gamelib::core::Vec2 safeFeet; // the last floor stood on away from thorns
    int safeRoom = 0;
};

enum class EventKind {
    Jump, AirJump, WallJump, Dash, Land, Splash,
    Slash,      // value: aim
    SlashHit,   // the slash struck something solid or an enemy
    Pogo,
    Shoot,
    NoAmmo,
    Pound,      // the dive started
    PoundLand,
    Hurt,
    HazardHit,
    Death,
    EnemyHit, EnemyDown, EnemyShot, Bomb, Deflect,
    DoorOpen, DoorLocked,
    Break,      // value: BreakKind
    Shake,      // a collapsing floor starts to give
    Item,       // value: ItemKind
    Drop,       // value: DropKind
    Saved, Mapped,
    Scroll,     // value: the room entered
    AreaEnter,  // value: Area
    BossWake, BossHit, BossPhase, BossDown, BossLand, Strike,
    EscapeStart, Escaped,
};

struct Event {
    EventKind kind = EventKind::Jump;
    gamelib::core::Vec2 at;
    int value = 0;
    EnemyKind enemy = EnemyKind::Crawler;
    BossKind boss = BossKind::Sporemother;
    Ability ability = Ability::Count;
};

class Session {
public:
    explicit Session(Progress progress);

    void tick(const Input& input);
    // After Phase::Dead: back at the save station with full health (the world as it is).
    void respawn();
    // A save station used this tick: the caller writes Progress to the slot.
    [[nodiscard]] bool wantsSave() const { return m_wantsSave; }

    [[nodiscard]] Phase phase() const { return m_phase; }
    [[nodiscard]] int phaseTicks() const { return m_phaseTicks; }
    [[nodiscard]] const Progress& progress() const { return m_progress; }
    [[nodiscard]] const Hero& hero() const { return m_hero; }
    [[nodiscard]] const gamelib::core::TileMap& map() const { return m_map; }
    [[nodiscard]] gamelib::core::TileWorld tileWorld() const { return {m_map, 0, world().shapes()}; }
    [[nodiscard]] int roomIndex() const { return m_room; }
    [[nodiscard]] const RoomInfo& room() const { return world().room(m_room); }
    [[nodiscard]] Area area() const { return room().def->area; }
    // The room a scroll is going to (the current room when none is under way).
    [[nodiscard]] int scrollTarget() const { return m_scroll ? m_scroll->to : m_room; }
    [[nodiscard]] float scrollProgress(float alpha = 1.0f) const { return m_scroll ? m_scroll->progress(alpha) : 0.0f; }
    [[nodiscard]] std::span<const Enemy> enemies() const { return m_enemies; }
    [[nodiscard]] const Shots& shots() const { return m_shots; }
    [[nodiscard]] std::span<const Drop> drops() const { return m_drops; }
    [[nodiscard]] const Boss* boss() const { return m_boss ? &*m_boss : nullptr; }
    [[nodiscard]] bool doorOpen(int door) const;
    [[nodiscard]] bool broken(int breakable) const;
    [[nodiscard]] bool itemTaken(int item) const;
    // An item shows once its guardian is beaten (abilities) and until taken.
    [[nodiscard]] bool itemVisible(int item) const;
    [[nodiscard]] const gamelib::core::CrumbleTiles& collapse() const { return m_collapse; }
    // Seal tiles closing a guardian's room during the fight.
    [[nodiscard]] std::span<const gamelib::core::Cell> seals() const { return m_seals; }
    // The station (sign, tablet, save, map) the hero can use now, or -1.
    [[nodiscard]] int stationInReach() const;
    [[nodiscard]] std::optional<int> escapeTicksLeft() const;
    [[nodiscard]] Ability lastAbility() const { return m_lastAbility; }
    [[nodiscard]] int ticks() const { return m_ticks; }
    [[nodiscard]] std::span<const gamelib::core::Telegraphs<gamelib::core::Vec2>::Item> warnings() const
    {
        return m_warnings.items();
    }

    [[nodiscard]] gamelib::core::EventQueue<Event>& events() { return m_events; }
    [[nodiscard]] std::uint64_t hash() const;

    // Developer aids: abilities and keys, a start in another room.
    void grantAll();
    void debugStartIn(int room);

private:
    void applyFlags();
    void placeHero(gamelib::core::Vec2 feet);
    void enterRoom(int room, bool fromScroll);
    void spawnRoom();

    // session.cpp
    void playTick(const Input& input);
    void scrollTick();
    void checkRoomChange();
    void stepDrops();
    void stepShots();
    void touchItems();
    void useStation();
    void hurtHero(int damage, const gamelib::core::Rect& source);
    void hazardHit();
    void die();
    void openDoor(int door);
    void breakPatch(int patch);
    void strikeTiles(const gamelib::core::Rect& box, bool slash);
    void dropLoot(gamelib::core::Vec2 at, int chance);
    void sealRoom(bool sealed);
    void startEscape();

    // hero.cpp
    void stepHero(const Input& input);
    void stepSlash(const Input& input);
    void fire(const Input& input);
    void stepPound(const Input& input, bool& handled);

    // enemies.cpp
    void spawnEnemy(const SpawnDef& spawn);
    void stepEnemies();
    void stepEnemy(Enemy& enemy);
    void heroMeetsEnemies();
    void enemyHurt(Enemy& enemy, int damage, gamelib::core::Vec2 at);
    void enemyFire(gamelib::core::Vec2 at, gamelib::core::Vec2 velocity, int look, int damage, float gravity = 0.0f);

    // bosses.cpp
    void wakeBoss();
    void stepBoss();
    void bossHurt(int damage, gamelib::core::Vec2 at);
    void stepSporemother(Boss& boss);
    void stepTidewarden(Boss& boss);
    void stepColossus(Boss& boss);
    void stepPrismWyrm(Boss& boss);
    void stepHeart(Boss& boss);

    Progress m_progress;
    gamelib::core::TileMap m_map{1, 1, kTile};
    gamelib::core::CrumbleTiles m_collapse;
    int m_room = 0;
    Hero m_hero;
    std::vector<Enemy> m_enemies;
    Shots m_shots{96};
    std::vector<Drop> m_drops;
    std::optional<Boss> m_boss;
    std::vector<gamelib::core::Cell> m_seals;
    gamelib::core::Telegraphs<gamelib::core::Vec2> m_warnings;
    gamelib::core::Rng m_rng{0xCA7E};
    std::optional<gamelib::core::RoomScroll> m_scroll;
    Phase m_phase = Phase::Playing;
    int m_phaseTicks = 0;
    int m_ticks = 0;
    int m_escapeStart = -1;
    Ability m_lastAbility = Ability::Count;
    bool m_wantsSave = false;
    bool m_upHeld = false;
    gamelib::core::EventQueue<Event> m_events;
};

} // namespace cavern::game
