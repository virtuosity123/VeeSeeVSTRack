#include "Seq4.hpp"

Seq4Module::Seq4Module()
    : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
  cv = new SynthDevKit::CV(1.7f);
  currentStep = 0;
}

void Seq4Module::step() {
  float in = inputs[CLOCK_INPUT].value;
  cv->update(in);

  if (cv->newTrigger()) {
    float note = octives[(int)params[OCTAVE_PARAM].value] +
                 notes[(int)params[OCTAVE_PARAM + currentStep + 1].value];

    outputs[GATE_OUTPUT].value = note;

    for (int i = 0; i < 4; i++) {
      if (i == currentStep) {
        lights[i].value = 1.0;
      } else {
        lights[i].value = 0.0;
      }
    }
    currentStep++;

    if (currentStep == 4) {
      currentStep = 0;
    }
  }
}
