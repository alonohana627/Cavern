#pragma once

// The three save slots (GameLib's storage::SaveSlots: a versioned file per
// slot with a checksum and a backup) holding a game::Progress: the world
// flags, the map's fog, abilities, keys, upgrades, the save station, play
// time and deaths. The slot list shows each slot's summary line.

#include "session.hpp"

#include <gamelib/storage/save_slots.hpp>

#include <array>
#include <optional>
#include <string>

namespace cavern::app {

inline constexpr int kSlotCount = 3;

struct SlotView {
    bool exists = false;
    bool readable = false;
    std::string summary; // "62%  FUNGAL CAVES  1:02:13"
};

class Saves {
public:
    Saves();

    [[nodiscard]] SlotView view(int slot) const;
    // nullopt for an empty or unreadable slot.
    [[nodiscard]] std::optional<game::Progress> load(int slot);
    void save(int slot, const game::Progress& progress);
    void erase(int slot);

private:
    gamelib::storage::SaveSlots m_slots;
};

} // namespace cavern::app
