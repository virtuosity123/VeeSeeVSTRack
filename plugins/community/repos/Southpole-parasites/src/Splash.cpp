#include <string.h>
#include "Southpole.hpp"
#include "dsp/samplerate.hpp"
#include "dsp/digital.hpp"
#include "tides/generator.h"

namespace rack_plugin_Southpole_parasites {

struct Splash : Module {
	enum ParamIds {
		MODE_PARAM,
		RANGE_PARAM,

		FREQUENCY_PARAM,
		FM_PARAM,

		SHAPE_PARAM,
		SLOPE_PARAM,
		SMOOTHNESS_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		SHAPE_INPUT,
		SLOPE_INPUT,
		SMOOTHNESS_INPUT,

		TRIG_INPUT,
		FREEZE_INPUT,
		PITCH_INPUT,
		FM_INPUT,
		LEVEL_INPUT,

		CLOCK_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		HIGH_OUTPUT,
		LOW_OUTPUT,
		UNI_OUTPUT,
		BI_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		MODE_GREEN_LIGHT, MODE_RED_LIGHT,
		PHASE_GREEN_LIGHT, PHASE_RED_LIGHT,
		RANGE_GREEN_LIGHT, RANGE_RED_LIGHT,
		NUM_LIGHTS
	};

	bool sheep;
	tides_parasites::Generator generator;
	int frame = 0;
	uint8_t lastGate;
	SchmittTrigger modeTrigger;
	SchmittTrigger rangeTrigger;

	Splash();
	void step() override;


	void reset() override {
		generator.set_range(tides_parasites::GENERATOR_RANGE_MEDIUM);
		generator.set_mode(tides_parasites::GENERATOR_MODE_LOOPING);
		sheep = false;
	}

	void randomize() override {
		generator.set_range((tides_parasites::GeneratorRange) (randomu32() % 3));
		generator.set_mode((tides_parasites::GeneratorMode) (randomu32() % 3));
	}

	json_t *toJson() override {
		json_t *rootJ = json_object();

		json_object_set_new(rootJ, "mode", json_integer((int) generator.mode()));
		json_object_set_new(rootJ, "range", json_integer((int) generator.range()));
		json_object_set_new(rootJ, "sheep", json_boolean(sheep));
		json_object_set_new(rootJ, "featureMode", json_integer((int)generator.feature_mode_));

		return rootJ;
	}

	void fromJson(json_t *rootJ) override {
		json_t *featModeJ = json_object_get(rootJ, "featureMode");
		if(featModeJ)
		    generator.feature_mode_ = (tides_parasites::Generator::FeatureMode) json_integer_value(featModeJ);
		json_t *modeJ = json_object_get(rootJ, "mode");
		if (modeJ) {
			generator.set_mode((tides_parasites::GeneratorMode) json_integer_value(modeJ));
		}

		json_t *rangeJ = json_object_get(rootJ, "range");
		if (rangeJ) {
			generator.set_range((tides_parasites::GeneratorRange) json_integer_value(rangeJ));
		}

		json_t *sheepJ = json_object_get(rootJ, "sheep");
		if (sheepJ) {
			sheep = json_boolean_value(sheepJ);
		}
	}
};


Splash::Splash() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
	memset(&generator, 0, sizeof(generator));
	generator.Init();
	generator.set_sync(false);
	reset();
}

void Splash::step() {
	tides_parasites::GeneratorMode mode = generator.mode();
	if (modeTrigger.process(params[MODE_PARAM].value)) {
		mode = (tides_parasites::GeneratorMode) (((int)mode - 1 + 3) % 3);
		generator.set_mode(mode);
	}
	lights[MODE_GREEN_LIGHT].value = (mode == 2) ? 1.0 : 0.0;
	lights[MODE_GREEN_LIGHT].value = (mode == 0) ? 0.0 : 1.0;

	lights[MODE_RED_LIGHT].value = (mode == 0) ? 1.0 : 0.0;
	lights[MODE_RED_LIGHT].value = (mode == 2) ? 0.0 : 1.0;

	tides_parasites::GeneratorRange range = generator.range();
	if (rangeTrigger.process(params[RANGE_PARAM].value)) {
		range = (tides_parasites::GeneratorRange) (((int)range - 1 + 3) % 3);
		generator.set_range(range);
	}
	lights[RANGE_GREEN_LIGHT].value = (range == 2) ? 1.0 : 0.0;
	lights[RANGE_GREEN_LIGHT].value = (range == 0) ? 0.0 : 1.0;

	lights[RANGE_RED_LIGHT].value = (range == 0) ? 1.0 : 0.0;
	lights[RANGE_RED_LIGHT].value = (range == 2) ? 0.0 : 1.0;

	//Buffer loop
	if (generator.writable_block()) {
		// Pitch
		float pitch = params[FREQUENCY_PARAM].value;
		pitch += 12.0 * inputs[PITCH_INPUT].value;
		//pitch += params[FM_PARAM].value * inputs[FM_INPUT].normalize(0.1) / 5.0;
		float fm = clamp(inputs[FM_INPUT].value /5.0 * params[FM_PARAM].value /12.0f, -1.0f, 1.0f) * 0x600;
		
		pitch += 60.0;
		if (generator.feature_mode_ == tides_parasites::Generator::FEAT_MODE_HARMONIC) {
		    //this is probably not original but seems useful
		    pitch -= 12;
		    // Scale to the global sample rate
		    pitch += log2f(48000.0 / engineGetSampleRate()) * 12.0;
		    generator.set_pitch_high_range(clamp((int)pitch * 0x80, -0x8000, 0x7fff), fm);
		}
		else {
		    pitch += log2f(48000.0 / engineGetSampleRate()) * 12.0;
		    generator.set_pitch(clamp((int)pitch * 0x80, -0x8000, 0x7fff),fm);
		}

		if (generator.feature_mode_ == tides_parasites::Generator::FEAT_MODE_RANDOM) {
		    //TODO: should this be inverted?
		    generator.set_pulse_width(clamp(1.0 - params[FM_PARAM].value /12.0f, 0.0f, 2.0f) * 0x7fff);
		}
		
		// Slope, smoothness, pitch
		int16_t shape = clamp(params[SHAPE_PARAM].value + inputs[SHAPE_INPUT].value / 5.0f, -1.0f, 1.0f) * 0x7fff;
		int16_t slope = clamp(params[SLOPE_PARAM].value + inputs[SLOPE_INPUT].value / 5.0f, -1.0f, 1.0f) * 0x7fff;
		int16_t smoothness = clamp(params[SMOOTHNESS_PARAM].value + inputs[SMOOTHNESS_INPUT].value / 5.0f, -1.0f, 1.0f) * 0x7fff;
		generator.set_shape(shape);
		generator.set_slope(slope);
		generator.set_smoothness(smoothness);

		// Sync
		// Slight deviation from spec here.
		// Instead of toggling sync by holding the range button, just enable it if the clock port is plugged in.
		generator.set_sync(inputs[CLOCK_INPUT].active);	
		generator.FillBuffer();
#ifdef WAVETABLE_HACK
		generator.Process(sheep);
#endif
	}

	// Level
	uint16_t level = clamp(inputs[LEVEL_INPUT].normalize(8.0) / 8.0f, 0.0f, 1.0f) * 0xffff;
	if (level < 32)
		level = 0;

	uint8_t gate = 0;
	if (inputs[FREEZE_INPUT].value >= 0.7)
		gate |= tides_parasites::CONTROL_FREEZE;
	if (inputs[TRIG_INPUT].value >= 0.7)
		gate |= tides_parasites::CONTROL_GATE;
	if (inputs[CLOCK_INPUT].value >= 0.7)
		gate |= tides_parasites::CONTROL_CLOCK;
	if (!(lastGate & tides_parasites::CONTROL_CLOCK) && (gate & tides_parasites::CONTROL_CLOCK))
		gate |= tides_parasites::CONTROL_GATE_RISING;
	if (!(lastGate & tides_parasites::CONTROL_GATE) && (gate & tides_parasites::CONTROL_GATE))
		gate |= tides_parasites::CONTROL_GATE_RISING;
	if ((lastGate & tides_parasites::CONTROL_GATE) && !(gate & tides_parasites::CONTROL_GATE))
		gate |= tides_parasites::CONTROL_GATE_FALLING;
	lastGate = gate;

	const tides_parasites::GeneratorSample& sample = generator.Process(gate);
	
	uint32_t uni = sample.unipolar;
	int32_t bi = sample.bipolar;

	uni = uni * level >> 16;
	bi = -bi * level >> 16;
	float unif = (float) uni / 0xffff;
	float bif = (float) bi / 0x8000;

	outputs[HIGH_OUTPUT].value = sample.flags & tides_parasites::FLAG_END_OF_ATTACK ? 0.0 : 5.0;
	outputs[LOW_OUTPUT].value = sample.flags & tides_parasites::FLAG_END_OF_RELEASE ? 0.0 : 5.0;
	outputs[UNI_OUTPUT].value = unif * 8.0;
	outputs[BI_OUTPUT].value = bif * 5.0;

	if (sample.flags & tides_parasites::FLAG_END_OF_ATTACK)
		unif *= -1.0;
	lights[PHASE_GREEN_LIGHT].setBrightnessSmooth(fmaxf(0.0, unif));
	lights[PHASE_RED_LIGHT].setBrightnessSmooth(fmaxf(0.0, -unif));
}

struct SplashWidget : ModuleWidget {
	SVGPanel *panel0;
	SVGPanel *panel1;
	SVGPanel *panel2;
	void step() override;
	Menu *createContextMenu() override;

	SplashWidget(Splash *module) : ModuleWidget(module) {

	box.size = Vec(15 * 8, 380);

  	{
	  	panel0 = new SVGPanel();
	  	panel0->setBackground(SVG::load(assetPlugin(plugin, "res/Splash.svg")));
	  	panel0->box.size = box.size;
	  	addChild(panel0);
  	}
  	{
	  	panel1 = new SVGPanel();
	  	panel1->setBackground(SVG::load(assetPlugin(plugin, "res/TwoBumps.svg")));
	  	panel1->box.size = box.size;
	  	addChild(panel1);
  	}
  	{
	  	panel2 = new SVGPanel();
	  	panel2->setBackground(SVG::load(assetPlugin(plugin, "res/TwoDrunks.svg")));
	  	panel2->box.size = box.size;
	  	addChild(panel2);
  	}

  	const float x1 = 0.5*RACK_GRID_WIDTH;
  	const float x2 = 3.25*RACK_GRID_WIDTH;
  	const float x3 = 5.75*RACK_GRID_WIDTH; 
  	const float y1 = 40.0f;
  	const float y2 = 25.0f;
  	const float yh = 38.0f;


	addParam(ParamWidget::create<CKD6>(Vec(x3-3,y1-3), module, Splash::MODE_PARAM, 0.0, 1.0, 0.0));
	addChild(ModuleLightWidget::create<MediumLight<GreenRedLight>>(Vec(x3+7, y1+7), module, Splash::MODE_GREEN_LIGHT));

	addParam(ParamWidget::create<CKD6>(Vec(x3-3,y1+1.45*yh), module, Splash::RANGE_PARAM, 0.0, 1.0, 0.0));
	addChild(ModuleLightWidget::create<MediumLight<GreenRedLight>>(Vec(x3+7, y1+2*yh-10), module, Splash::RANGE_GREEN_LIGHT));

	addChild(ModuleLightWidget::create<MediumLight<GreenRedLight>>(Vec(x2-20, y2+2*yh), module, Splash::PHASE_GREEN_LIGHT));
	addParam(ParamWidget::create<sp_BlackKnob>(Vec(x2-7,y2+1.75*yh), module, Splash::FREQUENCY_PARAM, -48.0, 48.0, 0.0));

	addParam(ParamWidget::create<sp_SmallBlackKnob>(Vec(x3, y2+4*yh), module, Splash::SHAPE_PARAM, -1.0, 1.0, 0.0));
	addParam(ParamWidget::create<sp_SmallBlackKnob>(Vec(x3, y2+4.75*yh), module, Splash::SLOPE_PARAM, -1.0, 1.0, 0.0));
	addParam(ParamWidget::create<sp_SmallBlackKnob>(Vec(x3, y2+5.5*yh), module, Splash::SMOOTHNESS_PARAM, -1.0, 1.0, 0.0));


	addInput(Port::create<sp_Port>(Vec(x1, y1), Port::INPUT, module, Splash::TRIG_INPUT));
	addInput(Port::create<sp_Port>(Vec(x2, y1), Port::INPUT, module, Splash::FREEZE_INPUT));

	addInput(Port::create<sp_Port>(Vec(x1, y2+2*yh), Port::INPUT, module, Splash::PITCH_INPUT));
	addInput(Port::create<sp_Port>(Vec(x1,   y2+3.25*yh), Port::INPUT, module, Splash::FM_INPUT));
	addParam(ParamWidget::create<sp_Trimpot>(Vec(x2,y2+3.25*yh), module, Splash::FM_PARAM, -12.0, 12.0, 0.0));

	addInput(Port::create<sp_Port>(Vec(x1, y2+4*yh), Port::INPUT, module, Splash::SHAPE_INPUT));
	addInput(Port::create<sp_Port>(Vec(x1, y2+4.75*yh), Port::INPUT, module, Splash::SLOPE_INPUT));
	addInput(Port::create<sp_Port>(Vec(x1, y2+5.5*yh), Port::INPUT, module, Splash::SMOOTHNESS_INPUT));

	addInput(Port::create<sp_Port>(Vec(x3, y1+5.9*yh), Port::INPUT, module, Splash::LEVEL_INPUT));
	addInput(Port::create<sp_Port>(Vec(x1, y1+5.9*yh), Port::INPUT, module, Splash::CLOCK_INPUT));

	addOutput(Port::create<sp_Port>(Vec(x1, y1+7.125*yh), Port::OUTPUT, module, Splash::HIGH_OUTPUT));
	addOutput(Port::create<sp_Port>(Vec(x1+1*28., y1+7.125*yh), Port::OUTPUT, module, Splash::LOW_OUTPUT));
	addOutput(Port::create<sp_Port>(Vec(x1+2*28., y1+7.125*yh), Port::OUTPUT, module, Splash::UNI_OUTPUT));
	addOutput(Port::create<sp_Port>(Vec(x1+3*28., y1+7.125*yh), Port::OUTPUT, module, Splash::BI_OUTPUT));
	
}
};

void SplashWidget::step() {
	Splash *tides = dynamic_cast<Splash*>(module);
	assert(tides);

	if (tides->generator.feature_mode_ == tides_parasites::Generator::FEAT_MODE_HARMONIC) {

		panel0->visible = false;
		panel1->visible = true;
		panel2->visible = false;

	} else
	if (tides->generator.feature_mode_ == tides_parasites::Generator::FEAT_MODE_RANDOM) {
		panel0->visible = false;
		panel1->visible = false;
		panel2->visible = true;
	} else {
		panel0->visible = true;
		panel1->visible = false;
		panel2->visible = false;		
	}	
	ModuleWidget::step();
}

struct SplashSheepItem : MenuItem {
	Splash *tides;
	void onAction(EventAction &e) override {
		tides->sheep ^= true;
	}
	void step() override {
		rightText = (tides->sheep) ? "✔" : "";
		MenuItem::step();
	}
};

struct SplashModeItem : MenuItem {
	Splash *module;
	tides_parasites::Generator::FeatureMode mode;
    	void onAction(EventAction &e) override {
	  //module->playback = playback;
	    module->generator.feature_mode_ = mode;
	}
	void step() override {
	  rightText = (module->generator.feature_mode_ == mode) ? "✔" : "";
		MenuItem::step();
	}
};


Menu *SplashWidget::createContextMenu() {
	Menu *menu = ModuleWidget::createContextMenu();

	Splash *tides = dynamic_cast<Splash*>(module);
	assert(tides);

#ifdef WAVETABLE_HACK	
	menu->addChild(construct<MenuLabel>());
	menu->addChild(construct<SplashSheepItem>(&MenuLabel::text, "Sheep", &SplashSheepItem::tides, tides));
#endif
	menu->addChild(construct<MenuLabel>());
	menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Mode"));
	menu->addChild(construct<SplashModeItem>(&MenuItem::text, "Original", &SplashModeItem::module, tides, &SplashModeItem::mode, tides_parasites::Generator::FEAT_MODE_FUNCTION));
	menu->addChild(construct<SplashModeItem>(&MenuItem::text, "Harmonic", &SplashModeItem::module, tides, &SplashModeItem::mode, tides_parasites::Generator::FEAT_MODE_HARMONIC));
	menu->addChild(construct<SplashModeItem>(&MenuItem::text, "Random", &SplashModeItem::module, tides, &SplashModeItem::mode, tides_parasites::Generator::FEAT_MODE_RANDOM));
	return menu;
}

} // namespace rack_plugin_Southpole_parasites

using namespace rack_plugin_Southpole_parasites;

RACK_PLUGIN_MODEL_INIT(Southpole_parasites, Splash) {
   Model *modelSplash 	= Model::create<Splash,SplashWidget>(	 "Southpole Parasites", "Splash", 	"Splash / Lambs - tidal modulator", LFO_TAG, OSCILLATOR_TAG, WAVESHAPER_TAG, FUNCTION_GENERATOR_TAG);
   return modelSplash;
}
