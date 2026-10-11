// The words of the world: what the signs and lore tablets say. Action
// names in braces ({jump}) become the keys the player bound.
#include "world.hpp"

namespace cavern::game {

std::vector<SignText> signTexts()
{
    return {
        {"ruins.cliff", 0, "THE OLD WATCH-ROAD.\n{left} {right} WALK.   {jump} JUMP - HOLD IT TO LEAP HIGHER."},
        {"ruins.colonnade", 0, "CRAWLERS NEST IN THE RUINS.\n{slash} SLASHES. HOLD {up} TO SLASH UPWARD."},
        {"ruins.colonnade", 1, "THE SHRINE DOOR IS STUCK FAST. STRIKE IT TO FORCE IT OPEN."},
        {"fungal.shrine", 0,
         "A GLOWCAP SHRINE. STAND BEFORE IT AND PRESS {up} TO REST: YOUR WOUNDS CLOSE, YOUR BOLTS RETURN AND YOUR "
         "PATH IS WRITTEN DOWN."},
        {"fungal.hollow", 0,
         "{shoot} LOOSES A SPARK BOLT. BOLTS ARE FEW - SHRINES AND FALLEN FOES RESTORE THEM.\n{map} SHOWS THE MAP."},
    };
}

std::vector<RoomDef> floodedRooms()
{
    return {};
}

std::vector<RoomDef> crystalRooms()
{
    return {};
}

std::vector<RoomDef> coreRooms()
{
    return {};
}

} // namespace cavern::game
