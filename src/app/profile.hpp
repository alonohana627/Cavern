#pragma once

// What Cavern keeps outside the save slots, in cavern.ini in the user data
// folder (CAVERN_DATA_DIR moves it): GameLib's standard settings and bests
// (the fastest escape and the best completion), the key bindings ([keys],
// GameLib's action-binding text) and the slot played last.

#include "actions.hpp"

#include <gamelib/app/arcade_profile.hpp>
#include <gamelib/platform/actions.hpp>
#include <gamelib/storage/ini.hpp>

#include <string_view>

namespace cavern::app {

struct Prefs {
    gamelib::platform::ActionMap actions = makeActions();
    int lastSlot = 0; // 0..2

    void read(const gamelib::storage::IniFile& ini);
    void write(gamelib::storage::IniFile& ini) const;
    friend bool operator==(const Prefs&, const Prefs&) = default;
};

using ProfileFile = gamelib::app::ArcadeProfileFileOf<Prefs>;

[[nodiscard]] ProfileFile openProfile();

inline constexpr std::string_view kAppName = "Cavern";
inline constexpr std::string_view kDataDirVariable = "CAVERN_DATA_DIR";
// Bests: the fastest escape (ticks, lower is better) and the best completion (percent).
inline constexpr std::string_view kBestTime = "escape_ticks";
inline constexpr std::string_view kBestCompletion = "completion";

} // namespace cavern::app
