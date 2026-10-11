#pragma once

// The world of Cavern as data: about sixty rooms in five areas, each room a
// block of text placed on a grid of map cells (one cell = one screen of
// 32 x 18 tiles), from single screens to rooms several screens wide or
// tall. Rooms side by side join where both have an opening on the shared
// edge (GameLib's WorldGraph gives the edge exits, RoomScroll the
// transition). Everything the world holds - doors, cracked floors and
// walls, items, save and map stations, signs, enemies and guardians - is
// read from the room text at load, so a room is edited in one place.
//
// Tiles (the character is the tile id):
//   ' ' air            '#' rock               '*' glassy crystal (no wall cling)
//   '=' ledge (one-way, down + jump drops through)   '^' thorns (hurt, put back)
//   '~' water (swim)   ',' shallows (drawn as water, walked through)
//   '%' cracked floor (a ground pound breaks it)
//   'X' cracked wall (a slash or a shot breaks it)
//   'Q' a floor that collapses under the hero for good (the fall into the caves)
//   'L' lava (deadly in the core)
// Doors (one tile wide, a vertical run of door tiles is one door; door
// tiles on both sides of a room edge are one door seen from both rooms):
//   'D' plain (a slash or a shot opens it)   '1' green '2' blue '3' amber '4' violet (keys)
//   ')' opens only from its left side      '(' opens only from its right side
//   '|' sealed until the core's heart falls (the escape)
// Markers (air behind them):
//   'S' the start      '@' save station    'M' map station
//   '?' a sign         '!' a lore tablet   'E' the way out (the end of the escape)
//   'h' health upgrade 'o' ammo upgrade    'k' the area's key   'A' the area's ability
//   'B' the area's guardian (a boss)
//   enemies: 'c' crawler 'b' bat 't' sentry 'p' spore puff 'f' cap hopper 'r' creeper
//            'e' eel 's' shellback 'j' jelly 'd' shardling 'y' prism eye 'm' moth
//            'w' ember wisp 'q' core drone 'l' magma lobber

#include "rules.hpp"

#include <gamelib/core/grid.hpp>
#include <gamelib/core/room_grid.hpp>
#include <gamelib/core/tile_groups.hpp>
#include <gamelib/core/tile_groups.hpp>
#include <gamelib/core/math.hpp>
#include <gamelib/core/tile_collision.hpp>
#include <gamelib/core/tile_map.hpp>
#include <gamelib/core/world_graph.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace cavern::game {

enum class Area : std::uint8_t { Ruins, Fungal, Flooded, Crystal, Core, Count };
inline constexpr int kAreaCount = static_cast<int>(Area::Count);

[[nodiscard]] std::string_view areaName(Area area);
// The key each area hides, by door tile ('1'..'4'): fungal green, flooded
// blue, ruins amber, crystal violet; the core has none.
[[nodiscard]] int areaKey(Area area); // 0..3, or -1
[[nodiscard]] std::string_view keyName(int key);
// The ability an area's guardian guards, or Ability::Count (the core).
[[nodiscard]] Ability areaAbility(Area area);
[[nodiscard]] std::string_view abilityName(Ability ability);

// A room as written in the world files.
struct RoomDef {
    std::string_view id;   // "fungal.hollow": flags and signs name it
    std::string_view name; // "MOSSY HOLLOW"
    Area area = Area::Ruins;
    gamelib::core::Cell corner; // the top-left map cell
    gamelib::core::Cell size{1, 1}; // in map cells (screens)
    std::vector<std::string_view> rows; // size.y * 18 rows of size.x * 32 characters
};

// The text of a sign or tablet: the n-th '?' or '!' of a room (row-major).
struct SignText {
    std::string_view room;
    int index = 0;
    std::string_view text;
};

enum class EnemyKind : std::uint8_t {
    Crawler,   // walks its ledge
    Bat,       // hangs, then swoops on a wave towards the hero
    Sentry,    // a stone turret firing aimed bolts
    SporePuff, // floats up and down, bursts into spores when struck
    Hopper,    // a cap that hops at the hero
    Creeper,   // walks the ceiling and drops when the hero passes under
    Eel,       // leaps out of the water in arcs
    Shellback, // shielded from the front; turns to face the hero
    Jelly,     // drifts slowly towards the hero, stings
    Shardling, // charges along the floor once it sees the hero
    PrismEye,  // floats and fires a fan of beams
    Moth,      // a crystal moth that homes slowly
    Wisp,      // an ember circling its spot, darting at the hero
    Drone,     // hovers above the hero and drops bombs
    Lobber,    // throws arcing magma
    Count
};
inline constexpr int kEnemyKinds = static_cast<int>(EnemyKind::Count);

enum class BossKind : std::uint8_t { Sporemother, Tidewarden, Colossus, PrismWyrm, Heart, Count };
inline constexpr int kBossKinds = static_cast<int>(BossKind::Count);
[[nodiscard]] std::string_view bossName(BossKind boss);
[[nodiscard]] BossKind areaBoss(Area area);

enum class DoorKind : std::uint8_t { Plain, Key, OpensFromLeft, OpensFromRight, Escape };

struct Door {
    DoorKind kind = DoorKind::Plain;
    int key = -1;                            // Key: 0..3
    std::vector<gamelib::core::Cell> cells;  // top to bottom, left column first
    gamelib::core::TileId tile = 'D';
    std::string flag;                        // "door.<x>.<y>"
    gamelib::core::Rect box;                 // world units
};

enum class BreakKind : std::uint8_t { Floor, Wall, Collapse };

// A cracked floor, a cracked wall or a collapsing floor: one connected patch
// that breaks at once and stays broken.
struct Breakable {
    BreakKind kind = BreakKind::Floor;
    std::vector<gamelib::core::Cell> cells;
    std::string flag; // "broke.<x>.<y>"
    int room = 0;
};

enum class ItemKind : std::uint8_t { Health, Ammo, Key, Ability };

struct Item {
    ItemKind kind = ItemKind::Health;
    int key = -1;                         // Key: 0..3
    Ability ability = Ability::Count;     // Ability
    gamelib::core::Cell cell;
    int room = 0;
    std::string flag;                     // "item.<x>.<y>"
};

enum class StationKind : std::uint8_t { Save, Map, Sign, Tablet };

struct Station {
    StationKind kind = StationKind::Save;
    gamelib::core::Cell cell;
    int room = 0;
    std::string_view text; // signs and tablets
};

struct SpawnDef {
    EnemyKind kind = EnemyKind::Crawler;
    gamelib::core::Cell cell;
};

struct BossDef {
    BossKind kind = BossKind::Sporemother;
    gamelib::core::Cell cell;
    int room = 0;
    std::string flag; // "boss.<kind>"
};

// A passage between two rooms (for the map's links): the cells on either
// side of the shared edge, in map-cell units.
struct Passage {
    int a = 0;
    int b = 0;
    gamelib::core::Vec2 from; // map cells
    gamelib::core::Vec2 to;
};

struct RoomInfo {
    const RoomDef* def = nullptr;
    gamelib::core::Rect bounds;     // world units
    gamelib::core::CellRange cells; // tiles
    std::vector<SpawnDef> spawns;
    int boss = -1;                  // index into World::bosses
    bool saveRoom = false;
};

// The whole world, read once from the room files. Throws
// std::invalid_argument naming the room and row for a room of the wrong
// size, rooms that overlap, an opening on a room edge facing rock or
// nothing, or a sign without text.
class World {
public:
    World();

    [[nodiscard]] const gamelib::core::TileMap& map() const { return m_map; }
    [[nodiscard]] const gamelib::core::TileShapes& shapes() const { return m_shapes; }
    [[nodiscard]] std::span<const RoomInfo> rooms() const { return m_rooms; }
    [[nodiscard]] const RoomInfo& room(const int index) const { return m_rooms[static_cast<std::size_t>(index)]; }
    [[nodiscard]] int roomCount() const { return static_cast<int>(m_rooms.size()); }
    [[nodiscard]] std::span<const gamelib::core::Rect> roomBounds() const { return m_roomBounds; }
    [[nodiscard]] const gamelib::core::WorldGraph& graph() const { return m_graph; }
    [[nodiscard]] int roomAt(gamelib::core::Vec2 point) const { return m_graph.roomAt(point); }
    [[nodiscard]] int roomOf(gamelib::core::Cell cell) const;
    [[nodiscard]] int roomById(std::string_view id) const;

    [[nodiscard]] std::span<const Door> doors() const { return m_doors; }
    [[nodiscard]] std::span<const Breakable> breakables() const { return m_breakables; }
    [[nodiscard]] std::span<const Item> items() const { return m_items; }
    [[nodiscard]] std::span<const Station> stations() const { return m_stations; }
    [[nodiscard]] std::span<const BossDef> bosses() const { return m_bosses; }
    [[nodiscard]] std::span<const Passage> passages() const { return m_passages; }
    [[nodiscard]] gamelib::core::Cell start() const { return m_start; }
    [[nodiscard]] gamelib::core::Cell exit() const { return m_exit; }
    // The map's size in map cells.
    [[nodiscard]] gamelib::core::Cell mapSize() const { return m_mapSize; }
    // The door or breakable holding a cell, or -1.
    [[nodiscard]] int doorAt(gamelib::core::Cell cell) const;
    [[nodiscard]] int breakableAt(gamelib::core::Cell cell) const;
    // Counts for the completion percentage.
    [[nodiscard]] int itemCount() const { return static_cast<int>(m_items.size()); }

private:
    void load();
    void scanRoom(int index);
    void groupObjects();

    gamelib::core::TileMap m_map{1, 1, kTile};
    gamelib::core::TileShapes m_shapes;
    std::vector<RoomInfo> m_rooms;
    std::vector<gamelib::core::Rect> m_roomBounds;
    gamelib::core::WorldGraph m_graph;
    std::vector<Door> m_doors;
    std::vector<Breakable> m_breakables;
    std::vector<Item> m_items;
    std::vector<Station> m_stations;
    std::vector<BossDef> m_bosses;
    std::vector<Passage> m_passages;
    gamelib::core::RoomGrid m_grid;
    gamelib::core::TileGroups m_doorGroups;
    gamelib::core::TileGroups m_breakGroups;
    gamelib::core::Cell m_start;
    gamelib::core::Cell m_exit;
    gamelib::core::Cell m_mapSize;
};

// The one world, built on first use.
[[nodiscard]] const World& world();

// The room files (world_*.cpp).
[[nodiscard]] std::vector<RoomDef> ruinsRooms();
[[nodiscard]] std::vector<RoomDef> fungalRooms();
[[nodiscard]] std::vector<RoomDef> floodedRooms();
[[nodiscard]] std::vector<RoomDef> crystalRooms();
[[nodiscard]] std::vector<RoomDef> coreRooms();
[[nodiscard]] std::vector<SignText> signTexts();

// True for the tiles of a door (plain, keyed, one-way, the escape seal).
[[nodiscard]] bool isDoorTile(gamelib::core::TileId id);

} // namespace cavern::game
