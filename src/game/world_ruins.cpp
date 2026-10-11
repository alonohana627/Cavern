// The surface ruins: the cliff road where the hero arrives, the colonnade
// and the sunken court whose floor gives way into the caves - and, reached
// again later by climbing back up, the bell tower, the ramparts, the
// library and the Gate Colossus guarding the quake stomp.
#include "world.hpp"

namespace cavern::game {

std::vector<RoomDef> ruinsRooms()
{
    std::vector<RoomDef> rooms;
    rooms.push_back({.id = "ruins.cliff", .name = "WIND-WORN CLIFF", .area = Area::Ruins, .corner = {0, 1}, .size = {1, 1},
                     .rows = {
                         "#                               ",
                         "#                               ",
                         "#                               ",
                         "#                               ",
                         "#                               ",
                         "#                               ",
                         "#                               ",
                         "#                               ",
                         "#                               ",
                         "##                              ",
                         "###                    =====    ",
                         "###                             ",
                         "####                            ",
                         "####  S    ?           ##       ",
                         "#############   ###########     ",
                         "#############^^^############    ",
                         "################################",
                         "################################",
                     }});
    rooms.push_back({.id = "ruins.colonnade", .name = "BROKEN COLONNADE", .area = Area::Ruins, .corner = {1, 1}, .size = {2, 1},
                     .rows = {
                         "                                                            ####",
                         "                                                           #####",
                         "                                                          ######",
                         "                                                          ######",
                         "                                                          ######",
                         "                                                          ######",
                         "                                                          ######",
                         "                       ==                                 ######",
                         "                                            =====         ######",
                         "                                                          ######",
                         "              ====                                        ######",
                         "                                     ====                 ######",
                         "                                                          ######",
                         "                    ##                                         D",
                         "        ##          ##              ##                 #       D",
                         "   ?    ##    c     ##             ###   c    ^^^     ###   ?  D",
                         "################################################################",
                         "################################################################",
                     }});
    rooms.push_back({.id = "ruins.court", .name = "SUNKEN COURT", .area = Area::Ruins, .corner = {3, 1}, .size = {1, 1},
                     .rows = {
                         "################################",
                         "#######              ###########",
                         "####                    ########",
                         "##                          ####",
                         "#                             ##",
                         "#                             ##",
                         "#                          #####",
                         "#                         ######",
                         "#                        #######",
                         "#                        #######",
                         "#                        #######",
                         "#                        #######",
                         "##                       #######",
                         "D                        #######",
                         "D                        #######",
                         "D                        #######",
                         "##QQQQQQQQQQQQQQQQQQQQQQQ#######",
                         "##QQQQQQQQQQQQQQQQQQQQQQQ#######",
                     }});
    return rooms;
}

} // namespace cavern::game
