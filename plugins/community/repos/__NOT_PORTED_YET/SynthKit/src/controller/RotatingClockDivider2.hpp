#include <cstdint>

#include "../../deps/SynthDevKit/src/CV.hpp"
#include "../../deps/SynthDevKit/src/Clock.hpp"
#include "../SynthKit.hpp"

struct RotatingClockDivider2Module : Module {
  enum ParamIds { NUM_PARAMS };
  enum InputIds { TOP_INPUT, ROTATE_INPUT, NUM_INPUTS };
  enum OutputIds {
    FIRST_OUTPUT,
    SECOND_OUTPUT,
    THIRD_OUTPUT,
    FOURTH_OUTPUT,
    FIFTH_OUTPUT,
    SIXTH_OUTPUT,
    SEVENTH_OUTPUT,
    EIGHTH_OUTPUT,
    NUM_OUTPUTS
  };
  enum LightIds {
    FIRST_LED,
    SECOND_LED,
    THIRD_LED,
    FOURTH_LED,
    FIFTH_LED,
    SIXTH_LED,
    SEVENTH_LED,
    EIGHTH_LED,
    ROTATE_LED,
    NUM_LIGHTS
  };

  RotatingClockDivider2Module();
  void step() override;

  SynthDevKit::Clock *clock;
  SynthDevKit::CV *cv;
  int count;
};
