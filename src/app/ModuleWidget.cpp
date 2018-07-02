#include "global_pre.hpp"
#include "app.hpp"
#include "engine.hpp"
#include "plugin.hpp"
#include "window.hpp"
#include "global.hpp"
#include "global_ui.hpp"


namespace rack {


ModuleWidget::ModuleWidget(Module *module) {
	if (module) {
		engineAddModule(module);
	}
	this->module = module;
}

ModuleWidget::~ModuleWidget() {
	// Make sure WireWidget destructors are called *before* removing `module` from the rack.
	disconnect();
	// Remove and delete the Module instance
	if (module) {
		engineRemoveModule(module);
		delete module;
		module = NULL;
	}
}

void ModuleWidget::addInput(Port *input) {
	assert(input->type == Port::INPUT);
	inputs.push_back(input);
	addChild(input);
}

void ModuleWidget::addOutput(Port *output) {
	assert(output->type == Port::OUTPUT);
	outputs.push_back(output);
	addChild(output);
}

void ModuleWidget::addParam(ParamWidget *param) {
	params.push_back(param);
	addChild(param);
}

void ModuleWidget::setPanel(std::shared_ptr<SVG> svg) {
	// Remove old panel
	if (panel) {
		removeChild(panel);
		delete panel;
		panel = NULL;
	}

	panel = new SVGPanel();
	panel->setBackground(svg);
	addChild(panel);

	box.size = panel->box.size;
}


json_t *ModuleWidget::toJson() {
	json_t *rootJ = json_object();

	// plugin
	json_object_set_new(rootJ, "plugin", json_string(model->plugin->slug.c_str()));
	// version (of plugin)
	if (!model->plugin->version.empty())
		json_object_set_new(rootJ, "version", json_string(model->plugin->version.c_str()));

#ifdef USE_VST2
	json_t *vst2_unique_param_base_idJ = json_integer(module->vst2_unique_param_base_id);
	json_object_set_new(rootJ, "vst2_unique_param_base_id", vst2_unique_param_base_idJ);
#endif // USE_VST2

	// model
	json_object_set_new(rootJ, "model", json_string(model->slug.c_str()));
	// pos
	Vec pos = box.pos.div(RACK_GRID_SIZE).round();
	json_t *posJ = json_pack("[i, i]", (int) pos.x, (int) pos.y);
	json_object_set_new(rootJ, "pos", posJ);
	// params
	json_t *paramsJ = json_array();
	for (ParamWidget *paramWidget : params) {
		json_t *paramJ = paramWidget->toJson();
		json_array_append_new(paramsJ, paramJ);
	}
	json_object_set_new(rootJ, "params", paramsJ);
	// data
	if (module) {
		json_t *dataJ = module->toJson();
		if (dataJ) {
			json_object_set_new(rootJ, "data", dataJ);
		}
	}

	return rootJ;
}

void ModuleWidget::fromJson(json_t *rootJ) {
	// legacy
	int legacy = 0;
	json_t *legacyJ = json_object_get(rootJ, "legacy");
	if (legacyJ)
		legacy = json_integer_value(legacyJ);

#ifdef USE_VST2
   if(NULL != module) {
      json_t *vst2_unique_param_base_idJ = json_object_get(rootJ, "vst2_unique_param_base_id");
      if (vst2_unique_param_base_idJ) {
         module->vst2_unique_param_base_id = json_integer_value(vst2_unique_param_base_idJ);
         if((module->vst2_unique_param_base_id + module->params.size()) > global->vst2.next_unique_param_base_id) {
            global->vst2.next_unique_param_base_id = (module->vst2_unique_param_base_id + module->params.size());
         }
      }
   }
#endif // USE_VST2

	// pos
	json_t *posJ = json_object_get(rootJ, "pos");
	double x, y;
	json_unpack(posJ, "[F, F]", &x, &y);
	Vec pos = Vec(x, y);
	if (legacy && legacy <= 1) {
		box.pos = pos;
	}
	else {
		box.pos = pos.mult(RACK_GRID_SIZE);
	}

	// params
	json_t *paramsJ = json_object_get(rootJ, "params");
	size_t i;
	json_t *paramJ;
	json_array_foreach(paramsJ, i, paramJ) {
		if (legacy && legacy <= 1) {
			// Legacy 1 mode
			// The index in the array we're iterating is the index of the ParamWidget in the params vector.
			if (i < params.size()) {
				// Create upgraded version of param JSON object
				json_t *newParamJ = json_object();
				json_object_set(newParamJ, "value", paramJ);
				params[i]->fromJson(newParamJ);
				json_decref(newParamJ);
			}
		}
		else {
			// Get paramId
			json_t *paramIdJ = json_object_get(paramJ, "paramId");
			if (!paramIdJ)
				continue;
			int paramId = json_integer_value(paramIdJ);
			// Find ParamWidget(s) with paramId
			for (ParamWidget *paramWidget : params) {
				if (paramWidget->paramId == paramId)
					paramWidget->fromJson(paramJ);
			}
		}
	}

	// data
	json_t *dataJ = json_object_get(rootJ, "data");
	if (dataJ && module) {
		module->fromJson(dataJ);
	}
}

void ModuleWidget::disconnect() {
	for (Port *input : inputs) {
		global_ui->app.gRackWidget->wireContainer->removeAllWires(input);
	}
	for (Port *output : outputs) {
		global_ui->app.gRackWidget->wireContainer->removeAllWires(output);
	}
}

void ModuleWidget::create() {
}

void ModuleWidget::_delete() {
}

void ModuleWidget::reset() {
	for (ParamWidget *param : params) {
		param->reset();
	}
	if (module) {
		module->onReset();
	}
}

void ModuleWidget::randomize() {
	for (ParamWidget *param : params) {
		param->randomize();
	}
	if (module) {
		module->onRandomize();
	}
}

void ModuleWidget::draw(NVGcontext *vg) {
	nvgScissor(vg, 0, 0, box.size.x, box.size.y);
	Widget::draw(vg);

	// CPU meter
	if (module && global->gPowerMeter) {
		nvgBeginPath(vg);
		nvgRect(vg,
			0, box.size.y - 20,
			55, 20);
		nvgFillColor(vg, nvgRGBAf(0, 0, 0, 0.5));
		nvgFill(vg);

		std::string cpuText = stringf("%.0f mS", module->cpuTime * 1000.f);
		nvgFontFaceId(vg, global_ui->window.gGuiFont->handle);
		nvgFontSize(vg, 12);
		nvgFillColor(vg, nvgRGBf(1, 1, 1));
		nvgText(vg, 10.0, box.size.y - 6.0, cpuText.c_str(), NULL);

		float p = clamp(module->cpuTime, 0.f, 1.f);
		nvgBeginPath(vg);
		nvgRect(vg,
			0, (1.f - p) * box.size.y,
			5, p * box.size.y);
		nvgFillColor(vg, nvgRGBAf(1, 0, 0, 1.0));
		nvgFill(vg);
	}

	nvgResetScissor(vg);
}

void ModuleWidget::drawShadow(NVGcontext *vg) {
	nvgBeginPath(vg);
	float r = 20; // Blur radius
	float c = 20; // Corner radius
	Vec b = Vec(-10, 30); // Offset from each corner
	nvgRect(vg, b.x - r, b.y - r, box.size.x - 2*b.x + 2*r, box.size.y - 2*b.y + 2*r);
	NVGcolor shadowColor = nvgRGBAf(0, 0, 0, 0.2);
	NVGcolor transparentColor = nvgRGBAf(0, 0, 0, 0);
	nvgFillPaint(vg, nvgBoxGradient(vg, b.x, b.y, box.size.x - 2*b.x, box.size.y - 2*b.y, c, r, shadowColor, transparentColor));
	nvgFill(vg);
}

void ModuleWidget::onMouseDown(EventMouseDown &e) {
	Widget::onMouseDown(e);
	if (e.consumed)
		return;

	if (e.button == 1) {
		createContextMenu();
	}
	e.consumed = true;
	e.target = this;
}

void ModuleWidget::onMouseMove(EventMouseMove &e) {
	OpaqueWidget::onMouseMove(e);

	// Don't delete the ModuleWidget if a TextField is focused
	if (!global_ui->widgets.gFocusedWidget) {
		// Instead of checking key-down events, delete the module even if key-repeat hasn't fired yet and the cursor is hovering over the widget.
		if (glfwGetKey(global_ui->window.gWindow, GLFW_KEY_DELETE) == GLFW_PRESS || glfwGetKey(global_ui->window.gWindow, GLFW_KEY_BACKSPACE) == GLFW_PRESS) {
			if (!windowIsModPressed() && !windowIsShiftPressed()) {
				global_ui->app.gRackWidget->deleteModule(this);
				this->finalizeEvents();
				delete this;
				e.consumed = true;
				return;
			}
		}
	}
}

void ModuleWidget::onHoverKey(EventHoverKey &e) {
	switch (e.key) {
		case GLFW_KEY_I: {
			if (windowIsModPressed() && !windowIsShiftPressed()) {
				reset();
				e.consumed = true;
				return;
			}
		} break;
		case GLFW_KEY_R: {
			if (windowIsModPressed() && !windowIsShiftPressed()) {
				randomize();
				e.consumed = true;
				return;
			}
		} break;
		case GLFW_KEY_D: {
			if (windowIsModPressed() && !windowIsShiftPressed()) {
				global_ui->app.gRackWidget->cloneModule(this);
				e.consumed = true;
				return;
			}
		} break;
		case GLFW_KEY_U: {
			if (windowIsModPressed() && !windowIsShiftPressed()) {
				disconnect();
				e.consumed = true;
				return;
			}
		} break;
	}

	Widget::onHoverKey(e);
}

void ModuleWidget::onDragStart(EventDragStart &e) {
	dragPos = global_ui->app.gRackWidget->lastMousePos.minus(box.pos);
}

void ModuleWidget::onDragEnd(EventDragEnd &e) {
}

void ModuleWidget::onDragMove(EventDragMove &e) {
	if (!global_ui->app.gRackWidget->lockModules) {
		Rect newBox = box;
		newBox.pos = global_ui->app.gRackWidget->lastMousePos.minus(dragPos);
		global_ui->app.gRackWidget->requestModuleBoxNearest(this, newBox);
	}
}


struct DisconnectMenuItem : MenuItem {
	ModuleWidget *moduleWidget;
	void onAction(EventAction &e) override {
		moduleWidget->disconnect();
	}
};

struct ResetMenuItem : MenuItem {
	ModuleWidget *moduleWidget;
	void onAction(EventAction &e) override {
		moduleWidget->reset();
	}
};

struct RandomizeMenuItem : MenuItem {
	ModuleWidget *moduleWidget;
	void onAction(EventAction &e) override {
		moduleWidget->randomize();
	}
};

struct CloneMenuItem : MenuItem {
	ModuleWidget *moduleWidget;
	void onAction(EventAction &e) override {
		global_ui->app.gRackWidget->cloneModule(moduleWidget);
	}
};

struct DeleteMenuItem : MenuItem {
	ModuleWidget *moduleWidget;
	void onAction(EventAction &e) override {
		global_ui->app.gRackWidget->deleteModule(moduleWidget);
		moduleWidget->finalizeEvents();
		delete moduleWidget;
	}
};

Menu *ModuleWidget::createContextMenu() {
	Menu *menu = global_ui->ui.gScene->createMenu();

	MenuLabel *menuLabel = new MenuLabel();
	menuLabel->text = model->author + " " + model->name + " " + model->plugin->version;
	menu->addChild(menuLabel);

	ResetMenuItem *resetItem = new ResetMenuItem();
	resetItem->text = "Initialize";
	resetItem->rightText = WINDOW_MOD_KEY_NAME "+I";
	resetItem->moduleWidget = this;
	menu->addChild(resetItem);

	RandomizeMenuItem *randomizeItem = new RandomizeMenuItem();
	randomizeItem->text = "Randomize";
	randomizeItem->rightText = WINDOW_MOD_KEY_NAME "+R";
	randomizeItem->moduleWidget = this;
	menu->addChild(randomizeItem);

	DisconnectMenuItem *disconnectItem = new DisconnectMenuItem();
	disconnectItem->text = "Disconnect cables";
	disconnectItem->rightText = WINDOW_MOD_KEY_NAME "+U";
	disconnectItem->moduleWidget = this;
	menu->addChild(disconnectItem);

	CloneMenuItem *cloneItem = new CloneMenuItem();
	cloneItem->text = "Duplicate";
	cloneItem->rightText = WINDOW_MOD_KEY_NAME "+D";
	cloneItem->moduleWidget = this;
	menu->addChild(cloneItem);

	DeleteMenuItem *deleteItem = new DeleteMenuItem();
	deleteItem->text = "Delete";
	deleteItem->rightText = "Backspace/Delete";
	deleteItem->moduleWidget = this;
	menu->addChild(deleteItem);

	appendContextMenu(menu);

	return menu;
}


} // namespace rack
