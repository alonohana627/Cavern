#include "demo.hpp"

#include <string_view>

namespace cavern::game {

namespace {

// helpers

// Recorded by playing the first rooms; see AGENTS.md ("Recording the demo").
constexpr std::string_view kDemo = R"(gamelib-replay 1
version 1
seed 0
ticks 60
inputs
60 .......
)";

} // namespace

std::shared_ptr<const gamelib::core::InputRecording<Input>> demoRecording()
{
    static const std::shared_ptr<const gamelib::core::InputRecording<Input>> recording = [] {
        auto parsed = gamelib::core::InputRecording<Input>::parse(kDemo, inputCodec().decoder());
        return std::make_shared<const gamelib::core::InputRecording<Input>>(
            parsed ? std::move(*parsed) : gamelib::core::InputRecording<Input>{});
    }();
    return recording;
}

} // namespace cavern::game
