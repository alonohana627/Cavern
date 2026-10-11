#include "world.hpp"

#include <algorithm>
#include <stdexcept>

namespace cavern::game {

namespace {

// helpers

using gamelib::core::Cell;
using gamelib::core::Rect;
using gamelib::core::TileId;
using gamelib::core::TileShape;
using gamelib::core::Vec2;


const std::vector<RoomDef>& _defs()
{
    static const std::vector<RoomDef> defs = [] {
        std::vector<RoomDef> all;
        for (auto list : {ruinsRooms(), fungalRooms(), floodedRooms(), crystalRooms(), coreRooms()}) {
            for (RoomDef& def : list) {
                all.push_back(std::move(def));
            }
        }
        return all;
    }();
    return defs;
}

[[noreturn]] void _fail(const RoomDef& def, const std::string& what)
{
    throw std::invalid_argument("room " + std::string{def.id} + ": " + what);
}

bool _isMarker(const char c)
{
    switch (c) {
    case 'S': case '@': case 'M': case '?': case '!': case 'E': case 'h': case 'o': case 'k': case 'A': case 'B':
    case 'c': case 'b': case 't': case 'p': case 'f': case 'r': case 'e': case 's': case 'j': case 'd': case 'y':
    case 'm': case 'w': case 'q': case 'l': return true;
    default: return false;
    }
}

int _enemyKind(const char c)
{
    constexpr std::string_view kKinds = "cbtpfresjdymwql";
    const std::size_t at = kKinds.find(c);
    return at == std::string_view::npos ? -1 : static_cast<int>(at);
}

// Rock that never opens: an opening against it is a mistake.
bool _permanentRock(const TileId id)
{
    return id == '#' || id == '*';
}

// A tile that can be or become a way through.
bool _passable(const TileId id)
{
    return !_permanentRock(id);
}

std::string _cellName(const Cell c)
{
    return std::to_string(c.x) + "." + std::to_string(c.y);
}

} // namespace

std::string_view areaName(const Area area)
{
    switch (area) {
    case Area::Ruins: return "SURFACE RUINS";
    case Area::Fungal: return "FUNGAL CAVES";
    case Area::Flooded: return "FLOODED HALLS";
    case Area::Crystal: return "CRYSTAL DEPTHS";
    case Area::Core: return "THE CORE";
    case Area::Count: break;
    }
    return "";
}

int areaKey(const Area area)
{
    switch (area) {
    case Area::Fungal: return 0;
    case Area::Flooded: return 1;
    case Area::Ruins: return 2;
    case Area::Crystal: return 3;
    default: return -1;
    }
}

std::string_view keyName(const int key)
{
    switch (key) {
    case 0: return "GREEN KEY";
    case 1: return "BLUE KEY";
    case 2: return "AMBER KEY";
    case 3: return "VIOLET KEY";
    default: return "KEY";
    }
}

Ability areaAbility(const Area area)
{
    switch (area) {
    case Area::Fungal: return Ability::Dash;
    case Area::Flooded: return Ability::WallJump;
    case Area::Ruins: return Ability::GroundPound;
    case Area::Crystal: return Ability::DoubleJump;
    default: return Ability::Count;
    }
}

std::string_view abilityName(const Ability ability)
{
    switch (ability) {
    case Ability::Dash: return "SHADOW DASH";
    case Ability::WallJump: return "CLING CLAWS";
    case Ability::GroundPound: return "QUAKE STOMP";
    case Ability::DoubleJump: return "ECHO WINGS";
    case Ability::Count: break;
    }
    return "";
}

std::string_view bossName(const BossKind boss)
{
    switch (boss) {
    case BossKind::Sporemother: return "THE SPOREMOTHER";
    case BossKind::Tidewarden: return "THE TIDEWARDEN";
    case BossKind::Colossus: return "THE GATE COLOSSUS";
    case BossKind::PrismWyrm: return "THE PRISM WYRM";
    case BossKind::Heart: return "THE MOLTEN HEART";
    case BossKind::Count: break;
    }
    return "";
}

BossKind areaBoss(const Area area)
{
    switch (area) {
    case Area::Fungal: return BossKind::Sporemother;
    case Area::Flooded: return BossKind::Tidewarden;
    case Area::Ruins: return BossKind::Colossus;
    case Area::Crystal: return BossKind::PrismWyrm;
    case Area::Core: return BossKind::Heart;
    case Area::Count: break;
    }
    return BossKind::Count;
}

bool isDoorTile(const TileId id)
{
    return id == 'D' || (id >= '1' && id <= '4') || id == ')' || id == '(' || id == '|';
}

World::World()
{
    m_shapes = gamelib::core::TileShapes{{'#', TileShape::Solid}, {'*', TileShape::Solid}, {'=', TileShape::OneWay},
                                         {'^', TileShape::Hazard}, {'L', TileShape::Hazard}, {'%', TileShape::Solid},
                                         {'X', TileShape::Solid},  {'Q', TileShape::Solid},  {'Z', TileShape::Solid},
                                         {'~', TileShape::Water}};
    m_shapes.setNoCling('*'); // glassy crystal: no wall slide or wall jump
    for (const char door : std::string_view{"D1234)(|"}) {
        m_shapes.set(gamelib::core::charTile(door), TileShape::Solid);
    }
    load();
}

int World::roomOf(const Cell cell) const
{
    return m_grid.roomOf(cell);
}

int World::roomById(const std::string_view id) const
{
    for (std::size_t i = 0; i < m_rooms.size(); ++i) {
        if (m_rooms[i].def->id == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int World::doorAt(const Cell cell) const
{
    return m_doorGroups.at(cell);
}

int World::breakableAt(const Cell cell) const
{
    return m_breakGroups.at(cell);
}

void World::load()
{
    // GameLib's room grid places the rooms and checks every edge; the
    // markers and objects are Cavern's.
    const std::vector<RoomDef>& defs = _defs();
    std::vector<gamelib::core::GridRoom> grid;
    grid.reserve(defs.size());
    for (const RoomDef& def : defs) {
        grid.push_back({.name = def.id, .corner = def.corner, .size = def.size, .rows = def.rows,
                        .openEdges = def.area == Area::Ruins ? gamelib::core::neighborBit::N : std::uint8_t{0}});
    }
    m_grid = gamelib::core::layoutGrid(grid, {kScreenColumns, kScreenRows}, '#',
                                       [](const char c) { return _passable(gamelib::core::charTile(c)); });
    m_mapSize = m_grid.mapSize();
    m_map = gamelib::core::TileMap{m_mapSize.x * kScreenColumns, m_mapSize.y * kScreenRows, kTile, 1,
                                   gamelib::core::charTile('#')};
    for (std::size_t i = 0; i < defs.size(); ++i) {
        RoomInfo info;
        info.def = &defs[i];
        info.cells = m_grid.rooms[i];
        info.bounds = gamelib::core::rangeRect(info.cells, kTile);
        m_rooms.push_back(std::move(info));
        m_roomBounds.push_back(m_rooms.back().bounds);
        (void)m_graph.addRoom(m_rooms.back().bounds);
        scanRoom(static_cast<int>(i));
    }
    for (const gamelib::core::GridPassage& p : m_grid.passages) {
        m_passages.push_back({.a = p.a, .b = p.b, .from = p.at - p.across * 0.25f, .to = p.at + p.across * 0.25f});
    }
    groupObjects();
}

void World::scanRoom(const int index)
{
    RoomInfo& info = m_rooms[static_cast<std::size_t>(index)];
    const RoomDef& def = *info.def;
    const int rows = def.size.y * kScreenRows;
    const int columns = def.size.x * kScreenColumns;
    const std::vector<SignText> texts = signTexts();
    int signs = 0;
    gamelib::core::TileLayer& layer = m_map.layer(0);
    for (int y = 0; y < rows; ++y) {
        const std::string_view row = def.rows[static_cast<std::size_t>(y)];
        for (int x = 0; x < columns; ++x) {
            const char c = row[static_cast<std::size_t>(x)];
            const Cell cell = info.cells.min + Cell{x, y};
            if (!_isMarker(c)) {
                layer.at(cell) = gamelib::core::charTile(c);
                continue;
            }
            // A marker leaves air behind it - or water (or shallows) when it sits in it.
            const char before = x > 0 ? row[static_cast<std::size_t>(x - 1)] : ' ';
            const char after = x + 1 < columns ? row[static_cast<std::size_t>(x + 1)] : ' ';
            const char under = before == '~' || after == '~' ? '~' : (before == ',' || after == ',' ? ',' : ' ');
            layer.at(cell) = gamelib::core::charTile(under);
            const std::string flag = "item." + _cellName(cell);
            switch (c) {
            case 'S': m_start = cell; break;
            case 'E': m_exit = cell; break;
            case '@':
                m_stations.push_back({.kind = StationKind::Save, .cell = cell, .room = index, .text = {}});
                info.saveRoom = true;
                break;
            case 'M': m_stations.push_back({.kind = StationKind::Map, .cell = cell, .room = index, .text = {}}); break;
            case '?':
            case '!': {
                const auto found = std::ranges::find_if(texts, [&](const SignText& t) {
                    return t.room == def.id && t.index == signs;
                });
                if (found == texts.end()) {
                    _fail(def, "sign " + std::to_string(signs) + " has no text");
                }
                m_stations.push_back({.kind = c == '?' ? StationKind::Sign : StationKind::Tablet, .cell = cell,
                                      .room = index, .text = found->text});
                ++signs;
                break;
            }
            case 'h': m_items.push_back({.kind = ItemKind::Health, .cell = cell, .room = index, .flag = flag}); break;
            case 'o': m_items.push_back({.kind = ItemKind::Ammo, .cell = cell, .room = index, .flag = flag}); break;
            case 'k':
                if (areaKey(def.area) < 0) {
                    _fail(def, "a key in an area without one");
                }
                m_items.push_back({.kind = ItemKind::Key, .key = areaKey(def.area), .cell = cell, .room = index, .flag = flag});
                break;
            case 'A':
                if (areaAbility(def.area) == Ability::Count) {
                    _fail(def, "an ability in an area without one");
                }
                m_items.push_back({.kind = ItemKind::Ability, .ability = areaAbility(def.area), .cell = cell, .room = index,
                                   .flag = flag});
                break;
            case 'B': {
                const BossKind kind = areaBoss(def.area);
                info.boss = static_cast<int>(m_bosses.size());
                m_bosses.push_back({.kind = kind, .cell = cell, .room = index,
                                    .flag = "boss." + std::to_string(static_cast<int>(kind))});
                break;
            }
            default:
                if (const int kind = _enemyKind(c); kind >= 0) {
                    info.spawns.push_back({.kind = static_cast<EnemyKind>(kind), .cell = cell});
                }
                break;
            }
        }
    }
    if (std::ranges::count_if(texts, [&](const SignText& t) { return t.room == def.id; }) != signs) {
        _fail(def, "has texts for signs it does not have");
    }
}

void World::groupObjects()
{
    // Doors and breakable patches are GameLib tile groups: runs of one tile.
    const gamelib::core::TileLayer& layer = m_map.layer(0);
    m_doorGroups = gamelib::core::TileGroups{layer, isDoorTile};
    for (int i = 0; i < m_doorGroups.size(); ++i) {
        const gamelib::core::TileGroup& g = m_doorGroups[i];
        Door door;
        door.tile = g.id;
        door.cells = g.cells;
        door.flag = m_doorGroups.flagName(i, "door");
        door.box = gamelib::core::rangeRect(g.range, kTile);
        switch (g.id) {
        case ')': door.kind = DoorKind::OpensFromLeft; break;
        case '(': door.kind = DoorKind::OpensFromRight; break;
        case '|': door.kind = DoorKind::Escape; break;
        case 'D': door.kind = DoorKind::Plain; break;
        default:
            door.kind = DoorKind::Key;
            door.key = g.id - '1';
            break;
        }
        m_doors.push_back(std::move(door));
    }
    m_breakGroups = gamelib::core::TileGroups{layer, [](const TileId id) { return id == '%' || id == 'X' || id == 'Q'; }};
    for (int i = 0; i < m_breakGroups.size(); ++i) {
        const gamelib::core::TileGroup& g = m_breakGroups[i];
        m_breakables.push_back({.kind = g.id == '%' ? BreakKind::Floor : (g.id == 'X' ? BreakKind::Wall : BreakKind::Collapse),
                                .cells = g.cells,
                                .flag = m_breakGroups.flagName(i, "broke"),
                                .room = roomOf(g.cells.front())});
    }
}

const World& world()
{
    static const World w;
    return w;
}

} // namespace cavern::game
