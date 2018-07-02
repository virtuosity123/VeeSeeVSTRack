//***********************************************************************************************
//Impromptu Modular: Modules for VCV Rack by Marc Boulé
//
//Based on code from the Fundamental and AudibleInstruments plugins by Andrew Belt 
//and graphics from the Component Library by Wes Milholen 
//See ./LICENSE.txt for all licenses
//See ./res/fonts/ for font licenses
//***********************************************************************************************

#ifndef IMPROMPU_MODULAR_HPP
#define IMPROMPU_MODULAR_HPP


#include "rack.hpp"
#include "IMWidgets.hpp"
#include <climits>

using namespace rack;


extern Plugin *plugin;

// All modules that are part of plugin go here
extern Model *modelTact;
extern Model *modelTwelveKey;
extern Model *modelClocked;
extern Model *modelPhraseSeq16;
extern Model *modelPhraseSeq32;
extern Model *modelGateSeq64;
extern Model *modelSemiModularSynth;
extern Model *modelWriteSeq32;
extern Model *modelWriteSeq64;
extern Model *modelBlankPanel;


// General constants
static const float lightLambda = 0.075f;
static const std::string lightPanelID = "Classic";
static const std::string darkPanelID = "Dark-valor";
static const std::string expansionMenuLabel = "Extra CVs (requires +4HP to the right!)";


// Constants for displaying notes

static const char noteLettersSharp[12] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
static const char noteLettersFlat [12] = {'C', 'D', 'D', 'E', 'E', 'F', 'G', 'G', 'A', 'A', 'B', 'B'};
static const char isBlackKey      [12] = { 0,   1,   0,   1,   0,   0,   1,   0,   1,   0,   1,   0 };


// Component offset constants

static const int hOffsetCKSS = 5;
static const int vOffsetCKSS = 2;
static const int vOffsetCKSSThree = -2;
static const int hOffsetCKSSH = 2;
static const int vOffsetCKSSH = 5;
static const int offsetCKD6 = -1;//does both h and v
static const int offsetCKD6b = 0;//does both h and v
static const int vOffsetDisplay = -2;
static const int offsetIMBigKnob = -6;//does both h and v
static const int offsetIMSmallKnob = 0;//does both h and v
static const int offsetMediumLight = 9;
static const float offsetLEDbutton = 3.0f;//does both h and v
static const float offsetLEDbuttonLight = 4.4f;//does both h and v
static const int offsetTL1105 = 4;//does both h and v
static const int offsetLEDbezel = 1;//does both h and v
static const float offsetLEDbezelLight = 2.2f;//does both h and v
static const int offsetTrimpot = 3;//does both h and v



// Variations on existing knobs, lights, etc


// Screws

struct IMScrew : DynamicSVGScrew {
	IMScrew() {
		addSVGalt(SVG::load(assetPlugin(plugin, "res/dark/comp/ScrewSilver.svg")));
	}
};


// Ports

struct IMPort : DynamicSVGPort {
	IMPort() {
		addFrame(SVG::load(assetGlobal("res/ComponentLibrary/PJ301M.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/dark/comp/PJ301M.svg")));
		shadow->blurRadius = 10.0;
		shadow->opacity = 0.8;
	}
};


// Buttons and switches

struct CKSSH : SVGSwitch, ToggleSwitch {
	CKSSH() {
		addFrame(SVG::load(assetPlugin(plugin, "res/comp/CKSSH_0.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/comp/CKSSH_1.svg")));
		sw->wrap();
		box.size = sw->box.size;
	}
};

struct CKSSThreeInv : SVGSwitch, ToggleSwitch {
	CKSSThreeInv() {
		addFrame(SVG::load(assetGlobal("res/ComponentLibrary/CKSSThree_2.svg")));
		addFrame(SVG::load(assetGlobal("res/ComponentLibrary/CKSSThree_1.svg")));
		addFrame(SVG::load(assetGlobal("res/ComponentLibrary/CKSSThree_0.svg")));
	}
};

struct IMBigPushButton : DynamicSVGSwitch, MomentarySwitch {
	IMBigPushButton() {
		addFrameAll(SVG::load(assetPlugin(plugin, "res/light/comp/CKD6b_0.svg")));
		addFrameAll(SVG::load(assetPlugin(plugin, "res/light/comp/CKD6b_1.svg")));
		addFrameAll(SVG::load(assetPlugin(plugin, "res/dark/comp/CKD6b_0.svg")));
		addFrameAll(SVG::load(assetPlugin(plugin, "res/dark/comp/CKD6b_1.svg")));	
	}
};


// Knobs

struct IMKnob : DynamicSVGKnob {
	IMKnob() {
		minAngle = -0.83*M_PI;
		maxAngle = 0.83*M_PI;
		shadow->blurRadius = 10.0;
		shadow->opacity = 0.8;
	}
};

struct IMBigKnob : IMKnob {
	IMBigKnob() {
		addFrameAll(SVG::load(assetPlugin(plugin, "res/light/comp/BlackKnobLargeWithMark.svg")));
		addFrameAll(SVG::load(assetPlugin(plugin, "res/dark/comp/BlackKnobLargeWithMark.svg")));
		addEffect(SVG::load(assetPlugin(plugin, "res/dark/comp/BlackKnobLargeWithMarkEffects.svg")));
	}
};
struct IMBigSnapKnob : IMBigKnob {
	IMBigSnapKnob() {
		snap = true;
		smooth = false;
	}
};

struct IMBigKnobInf : IMKnob {
	IMBigKnobInf() {
		addFrameAll(SVG::load(assetPlugin(plugin, "res/light/comp/BlackKnobLarge.svg")));
		addFrameAll(SVG::load(assetPlugin(plugin, "res/dark/comp/BlackKnobLarge.svg")));
		addEffect(SVG::load(assetPlugin(plugin, "res/dark/comp/BlackKnobLargeEffects.svg")));
		speed = 0.9f;				
		//smooth = false;
	}
};

struct IMSmallKnob : IMKnob {
	IMSmallKnob() {
		addFrameAll(SVG::load(assetPlugin(plugin, "res/light/comp/RoundSmallBlackKnob.svg")));
		addFrameAll(SVG::load(assetPlugin(plugin, "res/dark/comp/RoundSmallBlackKnob.svg")));
		addEffect(SVG::load(assetPlugin(plugin, "res/dark/comp/RoundSmallBlackKnobEffects.svg")));		
		shadow->box.pos = Vec(0.0, box.size.y * 0.15);
	}
};

struct IMSmallSnapKnob : IMSmallKnob {
	IMSmallSnapKnob() {
		snap = true;
		smooth = false;
	}
};

struct IMFivePosSmallKnob : IMSmallSnapKnob {
	IMFivePosSmallKnob() {
		minAngle = -0.5*M_PI;
		maxAngle = 0.5*M_PI;
	}
};

struct IMTactile : DynamicIMTactile {
	IMTactile() {
		smooth = false;// must be false or else DynamicIMTactile::changeValue() call from module will crash Rack
	}
};



// Lights

struct OrangeLight : GrayModuleLightWidget {
	OrangeLight() {
		addBaseColor(COLOR_ORANGE);
	}
};

template <typename BASE>
struct MuteLight : BASE {
	MuteLight() {
		this->box.size = mm2px(Vec(6.0f, 6.0f));
	}
};


// Other

struct InvisibleKey : MomentarySwitch {
	InvisibleKey() {
		box.size = Vec(34, 72);
	}
};

struct InvisibleKeySmall : MomentarySwitch {
	InvisibleKeySmall() {
		box.size = Vec(23, 50);
	}
};

struct ScrewSilverRandomRot : FramebufferWidget {// location: include/app.hpp and src/app/SVGScrew.cpp [some code also from src/app/SVGKnob.cpp]
	SVGWidget *sw;
	TransformWidget *tw;
	ScrewCircle *sc;
	ScrewSilverRandomRot();
};

struct ScrewHole : TransparentWidget {
	ScrewHole(Vec posGiven);
	void draw(NVGcontext *vg) override;
};	


enum RunModeIds {MODE_FWD, MODE_REV, MODE_PPG, MODE_BRN, MODE_RND, NUM_MODES};

NVGcolor prepareDisplay(NVGcontext *vg, Rect *box);
int moveIndex(int index, int indexNext, int numSteps);
bool moveIndexRunMode(int* index, int numSteps, int runMode, int* history);


#endif
