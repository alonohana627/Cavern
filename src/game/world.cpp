#include "world.hpp"

#include <algorithm>
#include <map>
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
                                         {'X', TileShape::Solid},  {'Q', TileShape::Solid},  {'Z', TileShape::Solid}};
    for (const char door : std::string_view{"D1234)(|"}) {
        m_shapes.set(gamelib::core::charTile(door), TileShape::Solid);
    }
    load();
}

int World::roomOf(const Cell cell) const
{
    const Cell mapCell{cell.x >= 0 ? cell.x / kScreenColumns : -1, cell.y >= 0 ? cell.y / kScreenRows : -1};
    return m_roomAt.inside(mapCell) ? m_roomAt.at(mapCell) : -1;
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
    return m_doorAt.inside(cell) ? m_doorAt.at(cell) : -1;
}

int World::breakableAt(const Cell cell) const
{
    return m_breakAt.inside(cell) ? m_breakAt.at(cell) : -1;
}

void World::load()
{
    const std::vector<RoomDef>& defs = _defs();
    Cell size{1, 1};
    for (const RoomDef& def : defs) {
        if (def.size.x < 1 || def.size.y < 1 || def.corner.x < 0 || def.corner.y < 0) {
            _fail(def, "bad corner or size");
        }
        size.x = std::max(size.x, def.corner.x + def.size.x);
        size.y = std::max(size.y, def.corner.y + def.size.y);
    }
    m_mapSize = size;
    m_roomAt = gamelib::core::Grid<int>{size.x, size.y, -1};
    m_map = gamelib::core::TileMap{size.x * kScreenColumns, size.y * kScreenRows, kTile, 1, gamelib::core::charTile('#')};
    m_doorAt = gamelib::core::Grid<int>{m_map.width(), m_map.height(), -1};
    m_breakAt = gamelib::core::Grid<int>{m_map.width(), m_map.height(), -1};

    for (const RoomDef& def : defs) {
        const int index = static_cast<int>(m_rooms.size());
        for (int y = 0; y < def.size.y; ++y) {
            for (int x = 0; x < def.size.x; ++x) {
                int& owner = m_roomAt.at(def.corner + Cell{x, y});
                if (owner >= 0) {
                    _fail(def, "overlaps room " + std::string{m_rooms[static_cast<std::size_t>(owner)].def->id});
                }
                owner = index;
            }
        }
        RoomInfo info;
        info.def = &def;
        info.cells = {.min = {def.corner.x * kScreenColumns, def.corner.y * kScreenRows},
                      .max = {(def.corner.x + def.size.x) * kScreenColumns - 1,
                              (def.corner.y + def.size.y) * kScreenRows - 1}};
        info.bounds = gamelib::core::rangeRect(info.cells, kTile);
        m_rooms.push_back(std::move(info));
        m_roomBounds.push_back(m_rooms.back().bounds);
        (void)m_graph.addRoom(m_rooms.back().bounds);
        scanRoom(index);
    }
    checkEdges();
    groupDoors();
    groupBreakables();
}

void World::scanRoom(const int index)
{
    RoomInfo& info = m_rooms[static_cast<std::size_t>(index)];
    const RoomDef& def = *info.def;
    const int rows = def.size.y * kScreenRows;
    const int columns = def.size.x * kScreenColumns;
    if (static_cast<int>(def.rows.size()) != rows) {
        _fail(def, "has " + std::to_string(def.rows.size()) + " rows, expected " + std::to_string(rows));
    }
    const std::vector<SignText> texts = signTexts();
    int signs = 0;
    gamelib::core::TileLayer& layer = m_map.layer(0);
    for (int y = 0; y < rows; ++y) {
        const std::string_view row = def.rows[static_cast<std::size_t>(y)];
        if (static_cast<int>(row.size()) != columns) {
            _fail(def, "row " + std::to_string(y) + " has " + std::to_string(row.size()) + " columns, expected "
                           + std::to_string(columns));
        }
        for (int x = 0; x < columns; ++x) {
            const char c = row[static_cast<std::size_t>(x)];
            const Cell cell = info.cells.min + Cell{x, y};
            if (!_isMarker(c)) {
                layer.at(cell) = gamelib::core::charTile(c);
                continue;
            }
            layer.at(cell) = gamelib::core::charTile(' ');
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

void World::checkEdges()
{
    const gamelib::core::TileLayer& layer = m_map.layer(0);
    // Pairs of cells across every room edge: an opening must face an opening
    // of the room behind it (or the sky, above a room at the top of the map).
    std::map<std::pair<int, int>, std::vector<std::pair<Cell, Cell>>> runs;
    for (std::size_t i = 0; i < m_rooms.size(); ++i) {
        const RoomInfo& room = m_rooms[i];
        const gamelib::core::CellRange r = room.cells;
        const auto check = [&](const Cell inside, const Cell outside) {
            const TileId here = layer.at(inside);
            if (!_passable(here)) {
                return;
            }
            const int other = roomOf(outside);
            if (other < 0 || !layer.inside(outside)) {
                if (outside.y < r.min.y && room.def->area == Area::Ruins) {
                    return; // the open sky
                }
                _fail(*room.def, "an opening at " + _cellName(inside - r.min) + " leads nowhere");
            }
            if (!_passable(layer.at(outside))) {
                _fail(*room.def, "an opening at " + _cellName(inside - r.min) + " faces rock in room "
                                     + std::string{m_rooms[static_cast<std::size_t>(other)].def->id});
            }
            if (static_cast<int>(i) < other) {
                runs[{static_cast<int>(i), other}].emplace_back(inside, outside);
            }
        };
        for (int x = r.min.x; x <= r.max.x; ++x) {
            check({x, r.min.y}, {x, r.min.y - 1});
            check({x, r.max.y}, {x, r.max.y + 1});
        }
        for (int y = r.min.y; y <= r.max.y; ++y) {
            check({r.min.x, y}, {r.min.x - 1, y});
            check({r.max.x, y}, {r.max.x + 1, y});
        }
    }
    // One map link per run of openings between two rooms, drawn across the edge.
    const auto toMap = [](const Vec2 cellCenter) {
        return Vec2{cellCenter.x / static_cast<float>(kScreenColumns), cellCenter.y / static_cast<float>(kScreenRows)};
    };
    for (const auto& [pair, cells] : runs) {
        std::size_t start = 0;
        for (std::size_t i = 1; i <= cells.size(); ++i) {
            const bool breaks = i == cells.size() || std::abs(cells[i].first.x - cells[i - 1].first.x)
                                                             + std::abs(cells[i].first.y - cells[i - 1].first.y) != 1;
            if (!breaks) {
                continue;
            }
            const Cell a = cells[(start + i - 1) / 2].first;
            const Cell b = cells[(start + i - 1) / 2].second;
            const Vec2 ca{static_cast<float>(a.x) + 0.5f, static_cast<float>(a.y) + 0.5f};
            const Vec2 cb{static_cast<float>(b.x) + 0.5f, static_cast<float>(b.y) + 0.5f};
            const Vec2 mid = (ca + cb) * 0.5f;
            const Vec2 dir = cb - ca;
            const Vec2 reach = dir * (0.22f * static_cast<float>(kScreenRows));
            m_passages.push_back({.a = pair.first, .b = pair.second, .from = toMap(mid - reach), .to = toMap(mid + reach)});
            start = i;
        }
    }
}

void World::groupDoors()
{
    gamelib::core::TileLayer& layer = m_map.layer(0);
    for (int y = 0; y < layer.height(); ++y) {
        for (int x = 0; x < layer.width(); ++x) {
            const Cell first{x, y};
            const TileId id = layer.at(first);
            if (!isDoorTile(id) || m_doorAt.at(first) >= 0) {
                continue;
            }
            Door door;
            door.tile = id;
            door.flag = "door." + _cellName(first);
            switch (id) {
            case ')': door.kind = DoorKind::OpensFromLeft; break;
            case '(': door.kind = DoorKind::OpensFromRight; break;
            case '|': door.kind = DoorKind::Escape; break;
            case 'D': door.kind = DoorKind::Plain; break;
            default:
                door.kind = DoorKind::Key;
                door.key = id - '1';
                break;
            }
            const int index = static_cast<int>(m_doors.size());
            // The connected run of the same tile: a door is one or two columns wide.
            std::vector<Cell> open{first};
            m_doorAt.at(first) = index;
            while (!open.empty()) {
                const Cell c = open.back();
                open.pop_back();
                door.cells.push_back(c);
                for (const Cell d : {Cell{1, 0}, Cell{-1, 0}, Cell{0, 1}, Cell{0, -1}}) {
                    const Cell n = c + d;
                    if (layer.inside(n) && layer.at(n) == id && m_doorAt.at(n) < 0) {
                        m_doorAt.at(n) = index;
                        open.push_back(n);
                    }
                }
            }
            std::ranges::sort(door.cells, [](const Cell a, const Cell b) { return a.x != b.x ? a.x < b.x : a.y < b.y; });
            Rect box = gamelib::core::cellRect(door.cells.front(), kTile);
            for (const Cell c : door.cells) {
                const Rect r = gamelib::core::cellRect(c, kTile);
                const float left = std::min(box.left(), r.left());
                const float top = std::min(box.top(), r.top());
                box = {left, top, std::max(box.right(), r.right()) - left, std::max(box.bottom(), r.bottom()) - top};
            }
            door.box = box;
            m_doors.push_back(std::move(door));
        }
    }
}

void World::groupBreakables()
{
    gamelib::core::TileLayer& layer = m_map.layer(0);
    for (int y = 0; y < layer.height(); ++y) {
        for (int x = 0; x < layer.width(); ++x) {
            const Cell first{x, y};
            const TileId id = layer.at(first);
            if ((id != '%' && id != 'X' && id != 'Q') || m_breakAt.at(first) >= 0) {
                continue;
            }
            Breakable patch;
            patch.kind = id == '%' ? BreakKind::Floor : (id == 'X' ? BreakKind::Wall : BreakKind::Collapse);
            patch.flag = "broke." + _cellName(first);
            patch.room = roomOf(first);
            const int index = static_cast<int>(m_breakables.size());
            std::vector<Cell> open{first};
            m_breakAt.at(first) = index;
            while (!open.empty()) {
                const Cell c = open.back();
                open.pop_back();
                patch.cells.push_back(c);
                for (const Cell d : {Cell{1, 0}, Cell{-1, 0}, Cell{0, 1}, Cell{0, -1}}) {
                    const Cell n = c + d;
                    if (layer.inside(n) && layer.at(n) == id && m_breakAt.at(n) < 0) {
                        m_breakAt.at(n) = index;
                        open.push_back(n);
                    }
                }
            }
            m_breakables.push_back(std::move(patch));
        }
    }
}

const World& world()
{
    static const World w;
    return w;
}

} // namespace cavern::game
