// The words of the world: what the signs and the lore tablets say. Action
// names in braces ({jump}) become the keys the player bound.
#include "world.hpp"

namespace cavern::game {

std::vector<SignText> signTexts()
{
    return {
        // The surface ruins.
        {"ruins.cliff", 0, "THE OLD WATCH-ROAD.\n{left} {right} WALK.   {jump} JUMP - HOLD IT TO LEAP HIGHER."},
        {"ruins.colonnade", 0,
         "CRAWLERS NEST IN THE RUINS.\n{slash} SLASHES. HOLD {up} TO SLASH UPWARD, {down} IN THE AIR TO STRIKE BELOW - "
         "AND BOUNCE OFF WHAT YOU HIT."},
        {"ruins.colonnade", 1, "THE SHRINE DOOR IS STUCK FAST. STRIKE IT TO FORCE IT OPEN."},
        {"ruins.belltower", 0,
         "THE BELL RANG WHEN THE GROUND FIRST SHOOK. THE WATCHERS WENT DOWN INTO THE DARK TO SILENCE WHAT STIRRED THERE. "
         "THE BELL HAS NOT RUNG SINCE."},
        {"ruins.shrine", 0, "THE EMBER SHRINE OF THE WATCH. BEYOND LIES THE GATE WHERE THE COLOSSUS STANDS - IT ANSWERS ONLY TO AMBER."},
        {"ruins.library", 0,
         "...FOUR GUARDIANS WE SET BELOW, EACH KEEPING A GIFT OF THE OLD WATCH, SO THAT ONLY ONE WHO EARNED ALL FOUR "
         "MIGHT REACH THE HEART..."},
        // The fungal caves.
        {"fungal.shrine", 0,
         "A GLOWCAP SHRINE. STAND BEFORE IT AND PRESS {up} TO REST: YOUR WOUNDS CLOSE, YOUR BOLTS RETURN AND YOUR PATH "
         "IS WRITTEN DOWN. FALL, AND YOU WAKE HERE."},
        {"fungal.hollow", 0, "{shoot} LOOSES A SPARK BOLT. BOLTS ARE FEW - SHRINES AND FALLEN FOES RESTORE THEM.\n{map} SHOWS THE MAP."},
        {"fungal.gallery", 0, "SOME WALLS ARE ONLY CRACKED ROCK. A FEW BLOWS WILL OPEN THEM."},
        {"fungal.pass", 0, "THIS DOOR HAS NO HANDLE ON THIS SIDE. IT OPENS ONLY FROM BEYOND."},
        {"fungal.mapcave", 0,
         "STUDY THE CHART BY THE LANTERN TO LEARN THE SHAPE OF THESE CAVES. EVERY REGION KEEPS ONE SUCH CHART."},
        {"fungal.deeps", 0,
         "THE MOTHER OF SPORES SLEEPS BEYOND THE GREEN DOOR. THE FLOOR HERE IS THIN - SOMETHING HEAVY COULD BREAK "
         "THROUGH IT."},
        // The flooded halls.
        {"flooded.tunnel", 0, "A DOOR WITHOUT A HANDLE ON THE FAR SIDE. FROM HERE IT SWINGS OPEN - A SHORTCUT TO THE GLOWCAP SHRINE."},
        {"flooded.shrine", 0,
         "THE TIDE SHRINE. IN DEEP WATER {jump} SWIMS UPWARD; HOLD IT AS YOU BREAK THE SURFACE TO LEAP ONTO THE BANK."},
        {"flooded.pump", 0, "THE PUMPS FAILED WHEN THE TIDEWARDEN WOKE. IT GUARDS THE CLAWS THAT LET A CLIMBER CLING TO STONE."},
        {"flooded.warden", 0, "THE TIDEWARDEN'S POOL. ITS DOOR ANSWERS ONLY TO BLUE."},
        // The crystal depths.
        {"crystal.descent", 0, "THE CRYSTAL HERE IS GLASS-SMOOTH. NO CLAW CAN HOLD IT. ONLY WINGS CARRY A TRAVELLER UP ITS CLIFFS."},
        {"crystal.shrine", 0, "THE QUARTZ SHRINE. THE DEPTHS SING AROUND IT."},
        {"crystal.geode", 0, "THE WYRM WAS BORN OF THE HEART'S FIRST LIGHT. IT WILL NOT LET THE WINGS GO WHILE IT STILL BREATHES."},
        {"crystal.antechamber", 0, "THE WYRM'S NEST. ITS DOOR ANSWERS ONLY TO VIOLET."},
        // The core.
        {"core.shrine", 0, "THE LAST SHRINE. BELOW, THE HEART OF THE WORLD BEATS LOUDER WITH EVERY STEP."},
        {"core.threshold", 0,
         "THE HEART BEATS BELOW. WHAT FALLS IN DOES NOT CLIMB OUT - NOT UNTIL THE HEART IS STILL, AND THEN THE WAY UP "
         "WILL NOT WAIT LONG."},
    };
}

} // namespace cavern::game
