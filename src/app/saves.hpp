#pragma once

// The three save slots (GameLib's storage::SaveSlots: a versioned file per
// slot with a checksum and a backup) holding a game::Progress: the world
// flags, the map's fog, abilities, keys, upgrades, the save station, play
// time and deaths. The slot list shows each slot's summary line.

#include "session.hpp"

#include <gamelib/storage/save_slots.hpp>
#include <gamelib/ui/save_slot_list.hpp>

#include <optional>
#include <vector>

namespace cavern::app {

inline constexpr int kSlotCount = 3;

class Saves {
public:
    Saves();

    // A row per slot for the slot list (summary "62%  FUNGAL CAVES  1:02:13").
    [[nodiscard]] std::vector<gamelib::ui::SlotRow> rows() const;
    // nullopt for an empty or unreadable slot.
    [[nodiscard]] std::optional<game::Progress> load(int slot);
    void save(int slot, const game::Progress& progress);
    void erase(int slot);

private:
    gamelib::storage::SaveSlots m_slots;
};

} // namespace cavern::app
