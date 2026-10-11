#include "saves.hpp"

#include "profile.hpp"

#include <gamelib/core/timers.hpp>
#include <gamelib/storage/files.hpp>

#include <algorithm>

namespace cavern::app {

namespace {

// helpers

constexpr int kSaveVersion = 1;

std::string _slotName(const int slot)
{
    return std::to_string(slot + 1);
}

gamelib::storage::IniFile _write(const game::Progress& p)
{
    gamelib::storage::IniFile ini;
    ini.setInt("hero", "abilities", p.abilities.bits);
    ini.setInt("hero", "keys", p.keys);
    ini.setInt("hero", "health_upgrades", p.healthUpgrades);
    ini.setInt("hero", "ammo_upgrades", p.ammoUpgrades);
    ini.setInt("hero", "station", p.saveStation);
    ini.setInt("run", "ticks", p.ticks);
    ini.setInt("run", "deaths", p.deaths);
    ini.setBool("run", "finished", p.finished);
    ini.set("map", "fog", p.reveal.encode());
    p.flags.forEach("", [&](const std::string_view name, const std::int64_t value) { ini.setInt("flags", name, value); });
    return ini;
}

game::Progress _read(const gamelib::storage::IniFile& ini)
{
    game::Progress p = game::Progress::fresh();
    p.abilities.bits = ini.getInt<std::uint8_t>("hero", "abilities", 0, 15).value_or(0);
    p.keys = ini.getInt<std::uint8_t>("hero", "keys", 0, 15).value_or(0);
    p.healthUpgrades = ini.getInt("hero", "health_upgrades", 0, 20).value_or(0);
    p.ammoUpgrades = ini.getInt("hero", "ammo_upgrades", 0, 20).value_or(0);
    p.saveStation = ini.getInt("hero", "station", -1, 1000).value_or(-1);
    if (p.saveStation >= static_cast<int>(game::world().stations().size())) {
        p.saveStation = -1;
    }
    p.ticks = std::max<long long>(0, ini.getInt("run", "ticks").value_or(0));
    p.deaths = ini.getInt("run", "deaths", 0, 1'000'000).value_or(0);
    p.finished = ini.getBool("run", "finished").value_or(false);
    if (const auto fog = ini.get("map", "fog")) {
        if (auto reveal = gamelib::core::MapReveal::decode(*fog);
            reveal && reveal->width() == p.reveal.width() && reveal->height() == p.reveal.height()) {
            p.reveal = *reveal;
        }
    }
    for (const auto& [name, value] : ini.entries("flags")) {
        if (!gamelib::core::WorldFlags::isValidName(name)) {
            continue;
        }
        if (const auto v = gamelib::storage::parseInt(value)) {
            p.flags.set(name, *v);
        }
    }
    return p;
}

std::string _summary(const game::Progress& p)
{
    const int station = p.saveStation;
    std::string where = "THE CLIFF ROAD";
    if (station >= 0) {
        const game::Station& s = game::world().stations()[static_cast<std::size_t>(station)];
        where = std::string{game::areaName(game::world().room(s.room).def->area)};
    }
    return std::to_string(p.completion()) + "%  " + where + "  "
           + gamelib::core::formatClock(static_cast<int>(std::min<std::int64_t>(p.ticks, 359'999 * 60)), game::kTickRate, 0)
           + (p.finished ? "  *" : "");
}

} // namespace

Saves::Saves()
    : m_slots{gamelib::storage::userDataDir(kAppName, kDataDirVariable) / "saves", kSaveVersion, "cavern"}
{
}

SlotView Saves::view(const int slot) const
{
    const gamelib::storage::SaveSlotInfo info = m_slots.info(_slotName(slot));
    return {.exists = info.exists, .readable = info.readable, .summary = info.summary};
}

std::optional<game::Progress> Saves::load(const int slot)
{
    auto loaded = m_slots.load(_slotName(slot));
    if (!loaded) {
        return std::nullopt;
    }
    return _read(loaded->data);
}

void Saves::save(const int slot, const game::Progress& progress)
{
    (void)m_slots.save(_slotName(slot), _write(progress), _summary(progress));
}

void Saves::erase(const int slot)
{
    (void)m_slots.remove(_slotName(slot));
}

} // namespace cavern::app
