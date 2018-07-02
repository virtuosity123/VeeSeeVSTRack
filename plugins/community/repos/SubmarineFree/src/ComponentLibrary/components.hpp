//////////////////
// Ports
//////////////////

struct sub_port : SVGPort {
	sub_port() {
		setSVG(SVG::load(assetPlugin(plugin, "res/Components/sub_port.svg")));
	}
};

struct sub_port_red : SVGPort {
	sub_port_red() {
		setSVG(SVG::load(assetPlugin(plugin, "res/Components/sub_port_red.svg")));
	}
};

struct sub_port_blue : SVGPort {
	sub_port_blue() {
		setSVG(SVG::load(assetPlugin(plugin, "res/Components/sub_port_blue.svg")));
	}
};

struct sub_port_black : SVGPort {
	sub_port_black() {
		setSVG(SVG::load(assetPlugin(plugin, "res/Components/sub_port_black.svg")));
	}
};

//////////////////
// Switches
//////////////////

struct sub_sw_2 : SVGSwitch, ToggleSwitch {
	sub_sw_2() {
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_2a.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_2b.svg")));
	}
};

struct sub_sw_3 : SVGSwitch, ToggleSwitch {
	sub_sw_3() {
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_3a.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_3b.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_3c.svg")));
	}
};

struct sub_sw_4 : SVGSwitch, ToggleSwitch {
	sub_sw_4() {
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_4a.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_4b.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_4c.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_4d.svg")));
	}
};

struct sub_sw_2h : SVGSwitch, ToggleSwitch {
	sub_sw_2h() {
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_2ha.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_2hb.svg")));
	}
};

struct sub_sw_3h : SVGSwitch, ToggleSwitch {
	sub_sw_3h() {
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_3ha.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_3hb.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_3hc.svg")));
	}
};

struct sub_sw_4h : SVGSwitch, ToggleSwitch {
	sub_sw_4h() {
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_4ha.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_4hb.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_4hc.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_sw_4hd.svg")));
	}
};

struct sub_btn : SVGSwitch, ToggleSwitch {
	sub_btn() {
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_btn.svg")));
		addFrame(SVG::load(assetPlugin(plugin, "res/Components/sub_btn_a.svg")));
	}
};

//////////////////
// Knobs
//////////////////

struct LightKnob : Knob {
	/** Angles in radians */
	float minAngle = -0.83*M_PI;
	float maxAngle = 0.83*M_PI;
	/** Radii in standard units */
	float radius = 19.0;
	int enabled = 1;
	LightKnob() {}
	void draw(NVGcontext *vg) override;
	void setEnabled(int val);
	void setRadius(int r);
};

struct sub_knob_small : LightKnob {
	sub_knob_small() {
		setRadius(12.0);	
	}
};

struct sub_knob_med : LightKnob {
	sub_knob_med() {
		setRadius(19.0);
	}
};

struct sub_knob_large : LightKnob {
	sub_knob_large() {
		setRadius(27.0);
	}
};

struct sub_knob_small_narrow : sub_knob_small {
	sub_knob_small_narrow() {
		minAngle = -0.75*M_PI;
		maxAngle = 0.75*M_PI;
	}
};

struct sub_knob_med_narrow : sub_knob_med {
	sub_knob_med_narrow() {
		minAngle = -0.75*M_PI;
		maxAngle = 0.75*M_PI;
	}
};

struct sub_knob_large_narrow : sub_knob_large {
	sub_knob_large_narrow() {
		minAngle = -0.75*M_PI;
		maxAngle = 0.75*M_PI;
	}
};

struct sub_knob_small_snap : sub_knob_small {
	sub_knob_small_snap() {
		snap = true;
		smooth = false;
	}
};

struct sub_knob_med_snap : sub_knob_med {
	sub_knob_med_snap() {
		snap = true;
		smooth = false;
	}
};

struct sub_knob_large_snap : sub_knob_large {
	sub_knob_large_snap() {
		snap = true;
		smooth = false;
	}
};

struct sub_knob_med_snap_narrow : sub_knob_med_snap {
	sub_knob_med_snap_narrow() {
		minAngle = -0.75*M_PI;
		maxAngle = 0.75*M_PI;
	}
};

//////////////////
// Lights
//////////////////

struct BlueRedLight : GrayModuleLightWidget {
	BlueRedLight() {
		addBaseColor(COLOR_BLUE);
		addBaseColor(COLOR_RED);
	}
};
