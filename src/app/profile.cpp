#include "profile.hpp"

#include <gamelib/app/bindings.hpp>

namespace cavern::app {

void Prefs::read(const gamelib::storage::IniFile& ini)
{
    lastSlot = ini.getInt("game", "last_slot", 0, 2).value_or(0);
    gamelib::app::readActionBindings(ini, actions);
}

void Prefs::write(gamelib::storage::IniFile& ini) const
{
    ini.setInt("game", "last_slot", lastSlot);
    gamelib::app::writeActionBindings(actions, ini);
}

ProfileFile openProfile()
{
    return gamelib::app::openArcadeProfile<Prefs>(kAppName, kDataDirVariable);
}

} // namespace cavern::app
