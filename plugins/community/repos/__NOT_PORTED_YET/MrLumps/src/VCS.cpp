// Some simple voltage controlled switches
// NEW in v0.5.2 SVG resizable panel graphics, onSampleRateChange bug fix

#include "MrLumps.hpp"
#include "dsp/digital.hpp"

//BG_IMAGE_FILE_1x8 replaced by SVG in v0.5.2
//BG_IMAGE_FILE_2x4 replaced by SVG in v0.5.2



struct VCS1x8 : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		TRIGGER_INPUT,
		SIGNAL_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SWITCHED_OUTPUT,
		NUM_OUTPUTS = SWITCHED_OUTPUT + 8
	};
	enum LightIds {
		OUTPUT_LIGHTS,
		NUM_LIGHTS = OUTPUT_LIGHTS + 8
	};

	SchmittTrigger inputTrigger;
	int currentSwitch;

	VCS1x8() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		currentSwitch = 0;
		lights[OUTPUT_LIGHTS + currentSwitch].value = 10.0f;
	}

	// Called via menu
	void onReset() override {
		for (int c = 0; c < 8; c++) {
			lights[OUTPUT_LIGHTS + c].value = 0.0f;
			outputs[c].value = 0.0f;
		}
		currentSwitch = 0;
		lights[OUTPUT_LIGHTS + currentSwitch].value = 10.0f;
	}

	//Todo
	void onRandomize() override {
	}

	void step() override;
};

//Lets try simplest 1st
//Just start at 0 and walk around the outputs on triggers
void VCS1x8::step() {

	if (inputs[TRIGGER_INPUT].active) {
		// External clock
		if (inputTrigger.process(inputs[TRIGGER_INPUT].value)) {
			//Switch ports to the next active port

			//Search forward from the current switch
			for (int i = currentSwitch+1; i < NUM_OUTPUTS; i++) {
				if (outputs[i].active) {
					lights[OUTPUT_LIGHTS + currentSwitch].value = 0.0f;
					outputs[currentSwitch].value = 0.0f;
					lights[OUTPUT_LIGHTS + i].value = 10.0f;
					currentSwitch = i;
					goto OUTPUT;
				}
			}
			//Else wrap around
			for (int i = 0; i < currentSwitch; i++) {
				if (outputs[i].active) {
					lights[OUTPUT_LIGHTS + currentSwitch].value = 0.0f;
					outputs[currentSwitch].value = 0.0f;
					lights[OUTPUT_LIGHTS + i].value = 10.0f;
					currentSwitch = i;
					goto OUTPUT;
				}
			}
			//Okay we have nothing to switch to
		}
	}


OUTPUT:

	//We have signal so send output
	if (inputs[SIGNAL_INPUT].active) {
		if (outputs[currentSwitch].active) {
			outputs[currentSwitch].value = inputs[SIGNAL_INPUT].value;
		}
	}

}

struct VCS1x8Widget : ModuleWidget {
	VCS1x8Widget(VCS1x8 *module) : ModuleWidget(module) {
		box.size = Vec(15 * 4, 380);

		{
		SVGPanel *panel = new SVGPanel();
		panel->box.size = box.size;
		panel->setBackground(SVG::load(assetPlugin(plugin, "res/VCS1x8.svg")));  // SVG panel graphic instead of PNG
		addChild(panel);
		}

		addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(15, 365)));

		const float bankX[8] = { 4, 31, 4, 31, 4, 31, 4, 31 };
		const float bankY[8] = { 112, 112, 179, 179, 246, 246, 313, 313 };

		//Trigger input
		addInput(Port::create<PJ3410Port>(Vec(29, 23), Port::INPUT, module, VCS1x8::TRIGGER_INPUT));

		//Signal input
		addInput(Port::create<PJ3410Port>(Vec(29, 57), Port::INPUT, module, VCS1x8::SIGNAL_INPUT));

		//Switched ouputs + lights
		for (int outputs = 0; outputs < 8; outputs++) {
			addChild(ModuleLightWidget::create<SmallLight<RedLight>>(Vec(bankX[outputs] + 9, bankY[outputs] - 12), module, VCS1x8::OUTPUT_LIGHTS + outputs));
			addOutput(Port::create<PJ301MPort>(Vec(bankX[outputs], bankY[outputs]), Port::OUTPUT, module, VCS1x8::SWITCHED_OUTPUT + outputs));
		}
	}
};







struct VCS2x4 : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		TRIGGER_INPUT,
		SIGNAL_INPUT_L,
		SIGNAL_INPUT_R,
		NUM_INPUTS
	};
	enum OutputIds {
		SWITCHED_OUTPUT_L,
		SWITCHED_OUTPUT_L2,
		SWITCHED_OUTPUT_L3,
		SWITCHED_OUTPUT_L4,
		SWITCHED_OUTPUT_R,
		SWITCHED_OUTPUT_R2,
		SWITCHED_OUTPUT_R3,
		SWITCHED_OUTPUT_R4,
		NUM_OUTPUTS
	};
	enum LightIds {
		OUTPUT_LIGHTS,
		NUM_LIGHTS = OUTPUT_LIGHTS + 8
	};

	SchmittTrigger inputTrigger;
	int currentSwitchL;
	int currentSwitchR;

	VCS2x4() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		currentSwitchL = 0;
		currentSwitchR = SWITCHED_OUTPUT_R;
		lights[OUTPUT_LIGHTS + currentSwitchL].value = 10.0f;
		lights[OUTPUT_LIGHTS + currentSwitchR].value = 10.0f;	
	}

	// Called via menu
	void onReset() override {
		for (int c = 0; c < 4; c++) {
			lights[OUTPUT_LIGHTS + c].value = 0.0f;   // Left
			lights[OUTPUT_LIGHTS + c+4].value = 0.0f; // Right	
		}
		outputs[currentSwitchL].value = 0.0f;
		outputs[currentSwitchR].value = 0.0f;
		currentSwitchL = 0;
		currentSwitchR = SWITCHED_OUTPUT_R;
		lights[OUTPUT_LIGHTS + currentSwitchL].value = 10.0f;
		lights[OUTPUT_LIGHTS + currentSwitchR].value = 10.0f;	
	}

	//Todo
	void onRandomize() override {
	}

	void step() override;
};

//Lets try simplest 1st
//Just start at 0 and walk around the outputs on triggers
void VCS2x4::step() {

	if (inputs[TRIGGER_INPUT].active) {
		// External clock
		if (inputTrigger.process(inputs[TRIGGER_INPUT].value)) {
			//Switch ports to the next active port


			// L Bank
			{
				//Search forward from the current switch
				for (int i = currentSwitchL + 1; i < NUM_OUTPUTS/2; i++) {
					if (outputs[i].active) {
						lights[OUTPUT_LIGHTS + currentSwitchL].value = 0.0f;
						outputs[currentSwitchL].value = 0.0f;
						lights[OUTPUT_LIGHTS + i].value = 10.0f;
						currentSwitchL = i;
						goto NEXTSIDE;
					}
				}
				//Else wrap around
				for (int i = SWITCHED_OUTPUT_L; i < currentSwitchL; i++) {
					if (outputs[i].active) {
						lights[OUTPUT_LIGHTS + currentSwitchL].value = 0.0f;
						outputs[currentSwitchL].value = 0.0f;
						lights[OUTPUT_LIGHTS + i].value = 10.0f;
						currentSwitchL = i;
						goto NEXTSIDE;
					}
				}
				//Okay we have nothing to switch to, next bank
			}
		NEXTSIDE:
			// R Bank
			{
				//Search forward from the current switch
				for (int i = currentSwitchR + 1; i < NUM_OUTPUTS; i++) {
					if (outputs[i].active) {
						lights[OUTPUT_LIGHTS + currentSwitchR].value = 0.0f;
						outputs[currentSwitchR].value = 0.0f;
						lights[OUTPUT_LIGHTS + i].value = 10.0f;
						currentSwitchR = i;
						goto OUTPUT;
					}
				}
				//Else wrap around
				for (int i = SWITCHED_OUTPUT_R; i < currentSwitchR; i++) {
					if (outputs[i].active) {
						lights[OUTPUT_LIGHTS + currentSwitchR].value = 0.0f;
						outputs[currentSwitchR].value = 0.0f;
						lights[OUTPUT_LIGHTS + i].value = 10.0f;
						currentSwitchR = i;
						goto OUTPUT;
					}
				}
				//Okay we have nothing to switch to
			}
		}
	}


OUTPUT:

	//We have signal so send output
	if (inputs[SIGNAL_INPUT_L].active) {
		if (outputs[currentSwitchL].active) {
			outputs[currentSwitchL].value = inputs[SIGNAL_INPUT_L].value;
		}
	}

	if (inputs[SIGNAL_INPUT_R].active) {
		if (outputs[currentSwitchR].active) {
			outputs[currentSwitchR].value = inputs[SIGNAL_INPUT_R].value;
		}
	}

}


struct VCS2x4Widget : ModuleWidget {
	VCS2x4Widget(VCS2x4 *module) : ModuleWidget(module) {
		box.size = Vec(15 * 4, 380);

		{
			SVGPanel *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(assetPlugin(plugin, "res/VCS2x4.svg")));  // SVG panel graphic instead of PNG
			addChild(panel);
		}

		addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(15, 365)));

		const float bankX[2] = { 4, 31 };
		const float bankY[4] = { 112, 179, 246, 313 };

		//Trigger input
		addInput(Port::create<PJ3410Port>(Vec(29, 23), Port::INPUT, module, VCS2x4::TRIGGER_INPUT));

		//Signal inputs
		addInput(Port::create<PJ3410Port>(Vec(0, 52), Port::INPUT, module, VCS2x4::SIGNAL_INPUT_L));
		addInput(Port::create<PJ3410Port>(Vec(29, 52), Port::INPUT, module, VCS2x4::SIGNAL_INPUT_R));

		//Switched ouputs + lights
		for (int outputs = 0; outputs < 4; outputs++) {
			addChild(ModuleLightWidget::create<SmallLight<RedLight>>(Vec(bankX[0] + 9, bankY[outputs] - 12), module, VCS2x4::OUTPUT_LIGHTS + outputs));
			addOutput(Port::create<PJ301MPort>(Vec(bankX[0], bankY[outputs]), Port::OUTPUT, module, VCS2x4::SWITCHED_OUTPUT_L + outputs));

			addChild(ModuleLightWidget::create<SmallLight<RedLight>>(Vec(bankX[1] + 9, bankY[outputs] - 12), module, VCS2x4::OUTPUT_LIGHTS + outputs+4));
			addOutput(Port::create<PJ301MPort>(Vec(bankX[1], bankY[outputs]), Port::OUTPUT, module, VCS2x4::SWITCHED_OUTPUT_R + outputs));
		}
	}
};

Model *modelVCS1x8 = Model::create<VCS1x8, VCS1x8Widget>("MrLumps", "VCS1", "1x8 Voltage Controlled Switch", SWITCH_TAG);
Model *modelVCS2x4 = Model::create<VCS2x4, VCS2x4Widget>("MrLumps", "VCS2", "2x4 Voltage Controlled Switch", SWITCH_TAG);
