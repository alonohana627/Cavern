#pragma once

// The attract demo: a recorded run through the first rooms of a new game
// (the cliff road, the colonnade and its door, the court whose floor gives
// way, and the first steps in the caves), played back on a fresh Session
// with GameLib's Playback. The recording is text (core::InputRecording,
// the seven-character inputs of rules.hpp) kept in demo.cpp.

#include "rules.hpp"

#include <gamelib/core/replay.hpp>

#include <memory>

namespace cavern::game {

// The demo's recording, parsed once (empty if the text no longer parses).
[[nodiscard]] std::shared_ptr<const gamelib::core::InputRecording<Input>> demoRecording();

} // namespace cavern::game
