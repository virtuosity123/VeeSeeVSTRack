#include "MrLumps.hpp"


Plugin *plugin;


void init(rack::Plugin *p) {
	plugin = p;
	p->slug = TOSTRING(SLUG);
	p->version = TOSTRING(VERSION);

	p->website = "https://github.com/djpeterso23662/MrLumps";
	p->manual = "https://github.com/djpeterso23662/MrLumps/blob/master/README.md";
	p->addModel(modelSEQEuclid);
	p->addModel(modelVCS1x8);
	p->addModel(modelVCS2x4);
}
