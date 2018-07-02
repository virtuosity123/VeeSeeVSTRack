// Copyright 2013 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Driver for the end of cycles output pins.

#ifndef TIDES_DRIVERS_GATE_INPUT_H_
#define TIDES_DRIVERS_GATE_INPUT_H_

#include "stmlib/stmlib.h"

#include <stm32f10x_conf.h>

#include "tides/generator.h"

namespace tides {

class GateInput {
 public:
  GateInput() { }
  ~GateInput() { }
  
  void Init();
  
  inline uint8_t Read() {
    uint16_t value = GPIOB->IDR;
    
    uint8_t state = 0;
    if (!(value & GPIO_Pin_1)) {
      state |= CONTROL_FREEZE;
    }
    if (!(value & GPIO_Pin_11)) {
      state |= CONTROL_GATE;
    }
    if (!(value & GPIO_Pin_10)) {
      state |= CONTROL_CLOCK;
    }
    if (!(previous_state_ & CONTROL_CLOCK) && (state & CONTROL_CLOCK)) {
      state |= CONTROL_CLOCK_RISING;
    }
    if (!(previous_state_ & CONTROL_GATE) && (state & CONTROL_GATE)) {
      state |= CONTROL_GATE_RISING;
    }
    if ((previous_state_ & CONTROL_GATE) && !(state & CONTROL_GATE)) {
      state |= CONTROL_GATE_FALLING;
    }
    previous_state_ = state;
    return state;
  }
  
 private:
  uint8_t previous_state_;
  
  DISALLOW_COPY_AND_ASSIGN(GateInput);
};

}  // namespace tides

#endif  // TIDES_DRIVERS_GATE_INPUT_H_
