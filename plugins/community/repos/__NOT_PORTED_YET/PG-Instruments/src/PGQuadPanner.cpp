#include "Template.hpp"

#define NUM_PANNERS 4
#define LEFT_MARGIN 20
#define TOP_MARGIN 100
#define SPACING 36

struct PGQuadPanner : Module
{
    enum ParamIds
    {
        ENUMS(LEVEL_PARAM, NUM_PANNERS),
        ENUMS(PAN_PARAM, NUM_PANNERS),
        NUM_PARAMS
    };
    
    enum InputIds
    {
        ENUMS(INPUT, NUM_PANNERS),
        NUM_INPUTS
    };
    
    enum OutputIds
    {
        LEFT_OUTPUT,
        RIGHT_OUTPUT,
        NUM_OUTPUTS
    };
    
    //float panning[NUM_PANNERS] = { 0.5f, 0.5f, 0.5f, 0.5f };
    
    PGQuadPanner() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, 0)
    {
    }
    
    void step() override
    {
        float left = 0.0f;
        float right = 0.0f;
        
        for(int i = 0; i < NUM_PANNERS; i++)
        {
            float input = inputs[INPUT + i].value;
            float panning = params[PAN_PARAM + i].value;
            float level = params[LEVEL_PARAM + i].value;
            
            float l, r;
            
            pan(input, panning, level, l, r);
            
            left += l;
            right += r;
        }
        
        outputs[LEFT_OUTPUT].value = left;
        outputs[RIGHT_OUTPUT].value = right;
    }
    
    void pan(float input, float panning, float level, float &left, float &right)
    {
        left = cosf(panning * M_PI / 2.0f) * input * level;
        right = sinf(panning * M_PI / 2.0f) * input * level;
    }
};

struct PGQuadPannerWidget : ModuleWidget
{
    PGQuadPannerWidget(PGQuadPanner *module) : ModuleWidget(module)
    {
        setPanel(SVG::load(assetPlugin(plugin, "res/PGQuadPanner.svg")));
        
  		addChild(Widget::create<ScrewSilver>(Vec(15, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(15, 365)));
		addChild(Widget::create<ScrewSilver>(Vec(box.size.x-30, 365)));

        for(int i = 0; i < NUM_PANNERS; i++)
        {
            addInput(Port::create<PJ301MPort>(Vec(LEFT_MARGIN, TOP_MARGIN + SPACING * i), Port::INPUT, module, PGQuadPanner::INPUT + i));
            addParam(ParamWidget::create<RoundBlackKnob>(Vec(LEFT_MARGIN + 40, TOP_MARGIN + SPACING * i), module, PGQuadPanner::PAN_PARAM + i, 0.0f, 1.0, 0.5f));
            addParam(ParamWidget::create<RoundBlackKnob>(Vec(LEFT_MARGIN + 80, TOP_MARGIN + SPACING * i), module, PGQuadPanner::LEVEL_PARAM + i, 0.0f, 1.0f, 0.7f));
        }
        
        addOutput(Port::create<PJ301MPort>(Vec(LEFT_MARGIN + 120, TOP_MARGIN + SPACING), Port::OUTPUT, module, PGQuadPanner::LEFT_OUTPUT));
        addOutput(Port::create<PJ301MPort>(Vec(LEFT_MARGIN + 120, TOP_MARGIN + SPACING * 2), Port::OUTPUT, module, PGQuadPanner::RIGHT_OUTPUT));
    }
};

Model *modelPGQuadPanner = Model::create<PGQuadPanner, PGQuadPannerWidget>("PG-Instruments", "PGQuadPanner", "PG Quad Panner", ATTENUATOR_TAG);
