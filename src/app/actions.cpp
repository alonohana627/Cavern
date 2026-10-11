#include "actions.hpp"

namespace cavern::app {

namespace {

// helpers

using gamelib::platform::GamepadButton;
using gamelib::platform::Key;

} // namespace

gamelib::platform::ActionMap makeActions()
{
    gamelib::platform::ActionMap map;
    map.add({.name = "left", .label = "LEFT", .category = "MOVE", .keys = {Key::Left, Key::A},
             .buttons = {GamepadButton::DpadLeft, GamepadButton::LeftStickLeft}});
    map.add({.name = "right", .label = "RIGHT", .category = "MOVE", .keys = {Key::Right, Key::D},
             .buttons = {GamepadButton::DpadRight, GamepadButton::LeftStickRight}});
    map.add({.name = "up", .label = "UP / USE", .category = "MOVE", .keys = {Key::Up, Key::W},
             .buttons = {GamepadButton::DpadUp, GamepadButton::LeftStickUp}});
    map.add({.name = "down", .label = "DOWN", .category = "MOVE", .keys = {Key::Down, Key::S},
             .buttons = {GamepadButton::DpadDown, GamepadButton::LeftStickDown}});
    map.add({.name = "jump", .label = "JUMP", .category = "ACTION", .keys = {Key::Z, Key::Space, Key::K},
             .buttons = {GamepadButton::A}});
    map.add({.name = "slash", .label = "SLASH", .category = "ACTION", .keys = {Key::X, Key::J},
             .buttons = {GamepadButton::X}});
    map.add({.name = "shoot", .label = "SHOOT", .category = "ACTION", .keys = {Key::C, Key::I},
             .buttons = {GamepadButton::Y}});
    map.add({.name = "dash", .label = "DASH", .category = "ACTION", .keys = {Key::LeftShift, Key::L},
             .buttons = {GamepadButton::RightBumper, GamepadButton::B}});
    map.add({.name = "map", .label = "MAP", .category = "SYSTEM", .keys = {Key::M, Key::Tab},
             .buttons = {GamepadButton::Back}});
    map.add({.name = "pause", .label = "PAUSE", .category = "SYSTEM", .keys = {Key::Escape, Key::P},
             .buttons = {GamepadButton::Start}, .locked = true});
    return map;
}

game::Input readInput(const gamelib::platform::ActionMap& actions, const gamelib::platform::InputState& input)
{
    game::Input in;
    const auto down = [&](const Action a) { return actions.isDown(input, actionId(a)); };
    const auto pressed = [&](const Action a) { return actions.wasPressed(input, actionId(a)); };
    in.move = (down(Action::Right) ? 1 : 0) - (down(Action::Left) ? 1 : 0);
    in.aim = (down(Action::Down) ? 1 : 0) - (down(Action::Up) ? 1 : 0);
    in.jumpPressed = pressed(Action::Jump);
    in.jumpHeld = in.jumpPressed || down(Action::Jump);
    in.slashPressed = pressed(Action::Slash);
    in.slashHeld = in.slashPressed || down(Action::Slash);
    in.shootPressed = pressed(Action::Shoot);
    in.shootHeld = in.shootPressed || down(Action::Shoot);
    in.dashPressed = pressed(Action::Dash);
    in.dashHeld = in.dashPressed || down(Action::Dash);
    in.interact = pressed(Action::Up);
    return in;
}

const gamelib::ui::MenuBindings& menuBindings()
{
    static const gamelib::ui::MenuBindings bindings{
        .up = {actionId(Action::Up)},
        .down = {actionId(Action::Down)},
        .left = {actionId(Action::Left)},
        .right = {actionId(Action::Right)},
        .confirm = {actionId(Action::Jump), actionId(Action::Slash)},
        .back = {actionId(Action::Pause)},
    };
    return bindings;
}

} // namespace cavern::app
