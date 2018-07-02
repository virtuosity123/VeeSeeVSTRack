#include "Southpole.hpp"

struct Blank1HPWidget : ModuleWidget {
	Blank1HPWidget(Module *module)  : ModuleWidget(module) {
	
		box.size = Vec(1 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
		{
			auto *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(assetPlugin(plugin, "res/sp-Blank2HP.svg")));
			addChild(panel);
		}
	}
};
struct Blank2HPWidget : ModuleWidget {
	Blank2HPWidget(Module *module)  : ModuleWidget(module) {
	
		box.size = Vec(2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
		{
			auto *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(assetPlugin(plugin, "res/sp-Blank2HP.svg")));
			addChild(panel);
		}
	}
};
struct Blank4HPWidget : ModuleWidget {
	Blank4HPWidget(Module *module)  : ModuleWidget(module) {
	
		box.size = Vec(4 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
		{
			auto *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(assetPlugin(plugin, "res/sp-Blank4HP.svg")));
			addChild(panel);
		}
	}
};
struct Blank8HPWidget : ModuleWidget {
	Blank8HPWidget(Module *module)  : ModuleWidget(module) {
	
		box.size = Vec(8 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
		{
			auto *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(assetPlugin(plugin, "res/sp-Blank8HP.svg")));
			addChild(panel);
		}
	}
};
struct Blank16HPWidget : ModuleWidget {
	Blank16HPWidget(Module *module)  : ModuleWidget(module) {
	
		box.size = Vec(16 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
		{
			auto *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(assetPlugin(plugin, "res/sp-Blank16HP.svg")));
			addChild(panel);
		}
	}
};
struct Blank42HPWidget : ModuleWidget {	
	Blank42HPWidget(Module *module)  : ModuleWidget(module) {
	
		box.size = Vec(42 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT);
		{
			auto *panel = new SVGPanel();
			panel->box.size = box.size;
			panel->setBackground(SVG::load(assetPlugin(plugin, "res/sp-Blank42HP.svg")));
			addChild(panel);
		}
	}
};

Model *modelBlank16HP 	= Model::create<Module,Blank16HPWidget>("Southpole", "Blank16HP",	"Blank 16 HP", BLANK_TAG);
Model *modelBlank1HP 	= Model::create<Module,Blank1HPWidget >("Southpole", "Blank1HP", 	"Blank 1 HP", BLANK_TAG);
Model *modelBlank2HP 	= Model::create<Module,Blank2HPWidget >("Southpole", "Blank2HP", 	"Blank 2 HP", BLANK_TAG);
Model *modelBlank42HP 	= Model::create<Module,Blank42HPWidget>("Southpole", "Blank42HP", 	"Blank 42 HP", BLANK_TAG);
Model *modelBlank4HP 	= Model::create<Module,Blank4HPWidget >("Southpole", "Blank4HP", 	"Blank 4 HP", BLANK_TAG);
Model *modelBlank8HP 	= Model::create<Module,Blank8HPWidget >("Southpole", "Blank8HP", 	"Blank 8 HP", BLANK_TAG);

