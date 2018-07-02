//***********************************************************************************************
//Chain-able clock module for VCV Rack by Marc Boulé
//
//Based on code from the Fundamental and Audible Instruments plugins by Andrew Belt and graphics  
//  from the Component Library by Wes Milholen. 
//See ./LICENSE.txt for all licenses
//See ./res/fonts/ for font licenses
//
//Module concept and design by Marc Boulé, Nigel Sixsmith and Xavier Belmont
//
//***********************************************************************************************


#include "ImpromptuModular.hpp"
#include "dsp/digital.hpp"


class Clock {
	// the 0.0d step does not actually to consume a sample step, it is used to reset every double-period so that lengths can be re-computed
	// a clock frame is defined as "length * iterations + syncWait";
	// for master, syncWait does not apply and iterations = 1
	
	double step;// 0.0d when stopped, [sampleTime to 2*period] for clock steps (*2 is because of swing, so we do groups of 2 periods)
	double length;// double period
	double sampleTime;
	int iterations;// run this many double periods before going into sync if sub-clock
	Clock* syncSrc = nullptr; // only subclocks will have this set to master clock
	static constexpr double guard = 0.0005;// in seconds, region for sync to occur right before end of length of last iteration; sub clocks must be low during this period
	
	public:
	
	bool isHigh(float swing, float pulseWidth) {
		// last 0.5ms (guard time) must be low so that sync mechanism will work properly (i.e. no missed pulses)
		//   this will automatically be the case, since code below disallows any pulses or inter-pulse times less than 1ms
		bool high = false;
		if (step > 0.0) {
			float swParam = swing;// swing is [-1 : 1]
			//swParam *= (swParam * (swParam > 0.0f ? 1.0f : -1.0f));// non-linear behavior for more sensitivity at center: f(x) = x^2 * sign(x)
			
			// all following values are in seconds
			float onems = 0.001f;
			float period = (float)length / 2.0f;
			float swing = (period - 2.0f * onems) * swParam;
			float p2min = onems;
			float p2max = period - onems - fabs(swing);
			if (p2max < p2min) {
				p2max = p2min;
			}
			
			//double p1 = sampleTime;// implicit, no need 
			double p2 = (double)((p2max - p2min) * pulseWidth + p2min);// pulseWidth is [0 : 1]
			double p3 = (double)(period + swing);
			double p4 = ((double)(period + swing)) + p2;
			
			if ( (step <= p2) || ((step > p3) && (step <= p4)) )
				high = true;
		}
		return high;
	}
	
	void setSync(Clock* clkGiven) {
		syncSrc = clkGiven;
	}
	
	inline void reset() {
		step = 0.0;
	}
	inline bool isReset() {
		return step == 0.0;
	}
	
	inline void setup(double lengthGiven, int iterationsGiven, double sampleTimeGiven) {
		length = lengthGiven;
		iterations = iterationsGiven;
		sampleTime = sampleTimeGiven;
	}
	
	inline void start() {
		step = sampleTime;
	}
	
	void stepClock() {// here the clock was output on step "step", this function is called at end of module::step()
		if (step > 0.0) {// if active clock
			step += sampleTime;
			if ( (syncSrc != nullptr) && (iterations == 1) && (step > (length - guard)) ) {// if in sync region
				if (syncSrc->isReset()) {
					reset();
				}// else nothing needs to be done, just wait and step stays the same
			}
			else {
				if (step > length) {// reached end iteration
					iterations--;
					step -= length;
					if (iterations <= 0) 
						reset();// frame done
				}
			}
		}
	}
	
	void applyNewLength(double lengthStretchFactor) {
		step *= lengthStretchFactor;
		length *= lengthStretchFactor;
	}
};


//*****************************************************************************


class ClockDelay {
	static const long maxSamples = 256000;// @ 30 BPM period is 2s, thus need at most 3/4 * 2s @ 192000 kHz = 288000 samples
	static const long entrySize = 64;
	static const long bufSize = maxSamples / entrySize;// = 4500 with 64 per uint64_t 
	uint64_t buffer[bufSize];
	
	long writeHead;// in samples, thus range is [0; maxSamples-1]
	bool finishedOnePass;
	
	public:
	
	void reset() {
		writeHead = 0l;
		finishedOnePass = false;
	}
	
	void write(bool value) {
		long bufIndex = writeHead / entrySize;
		uint64_t mask = ((uint64_t)1) << (uint64_t)(writeHead % entrySize);
		if (value)
			buffer[bufIndex] |= mask;
		else
			buffer[bufIndex] &= ~mask;
		
		writeHead++;
		if (writeHead >= maxSamples) {
			finishedOnePass = true;
			writeHead = 0;
		}
	}
	
	bool read(long delaySamples) {
		bool ret = false;
		long readHead = writeHead - delaySamples - 1;// -1 accounts for fact that when just written, writeHead was just moved (else read with 0 delay won't work)
		
		if (readHead < 0) {
			if (!finishedOnePass)
				readHead = -1l;
			else {
				readHead += maxSamples;// will now be positive
			}
		}
		
		if (readHead >= 0 && readHead < maxSamples) {
			long bufIndex = readHead / entrySize;
			uint64_t mask = ((uint64_t)1) << (uint64_t)(readHead % entrySize);
			ret = ((buffer[bufIndex] & mask) != 0);
		}
		
		return ret;
	}
};


//*****************************************************************************


struct Clocked : Module {
	enum ParamIds {
		ENUMS(RATIO_PARAMS, 4),// master is index 0
		ENUMS(SWING_PARAMS, 4),// master is index 0
		ENUMS(PW_PARAMS, 4),// master is index 0
		RESET_PARAM,
		RUN_PARAM,
		ENUMS(DELAY_PARAMS, 4),// index 0 is unused
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(PW_INPUTS, 4),// master is index 0
		RESET_INPUT,
		RUN_INPUT,
		BPM_INPUT,
		ENUMS(SWING_INPUTS, 4),// master is index 0
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(CLK_OUTPUTS, 4),// master is index 0
		RESET_OUTPUT,
		RUN_OUTPUT,
		BPM_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		RESET_LIGHT,
		RUN_LIGHT,
		ENUMS(CLK_LIGHTS, 4),// master is index 0
		NUM_LIGHTS
	};
	
	
	// Delay info
	// all fractions must be <= 0.75... or else delay buffer will overrun in worst case (192kHz, 30BPM)
	static const int numDelays = 8;
	std::string delayLabelsClock[numDelays] = {"  0", "/16",   "1/8",  "1/4", "1/3",     "1/2", "2/3",     "3/4"  };
	std::string delayLabelsNote[numDelays]  = {"  0", "/64",   "/32",  "/16", "/8t",     "1/8", "/4t",     "/8d"   };
	float delayValues[numDelays]            = {0.0f,  0.0625f, 0.125f, 0.25f, 1.0f/3.0f, 0.5f , 2.0f/3.0f, 0.75f};

	// Ratio info
	static const int numRatios = 33;
	float ratioValues[numRatios] = {1, 1.5, 2, 2.5, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 
									17, 19, 23, 29, 31, 32, 37, 41, 43, 47, 48, 53, 59, 61, 64};
	int ratioValuesDoubled[numRatios];// calculated only once in constructor

	// BPM info
	static const int bpmMax = 300;
	static const int bpmMin = 30;
	static const int bpmRange = bpmMax - bpmMin;
		
	// Knob utilities
	int getBeatsPerMinute(void) {
		int bpm = 0;
		if (inputs[BPM_INPUT].active) {
			bpm = (int) round( (inputs[BPM_INPUT].value / 10.0f) * (float)bpmRange + (float)bpmMin );// use round() in case negative voltage input
			bpm = clamp(bpm, bpmMin, bpmMax);
		}
		else {
			bpm = (int)(params[RATIO_PARAMS + 0].value + 0.5f);
		}
		return bpm;
	}
	int getRatioDoubled(int ratioKnobIndex) {
		// ratioKnobIndex is 0 for master BPM's ratio (mplicitly 1.0f), and 1 to 3 for other ratio knobs
		// returns a positive ratio for mult, negative ratio for div (0 never returned)
		int ret = 1;
		if (ratioKnobIndex > 0) {
			bool isDivision = false;
			int i = (int) round( params[RATIO_PARAMS + ratioKnobIndex].value );// [ -(numRatios-1) ; (numRatios-1) ]
			if (i < 0) {
				i *= -1;
				isDivision = true;
			}
			if (i >= numRatios) {
				i = numRatios - 1;
			}
			ret = ratioValuesDoubled[i];
			if (isDivision) 
				ret = -1l * ret;
		}
		return ret;
	}
	inline float calcRatio(int ratioDoubled) {
		// true float ratio, always positive, >1 for mult, <1 for div
		float ret = ((float)ratioDoubled) / 2.0f;
		if (ratioDoubled < 0)
			ret = 1.0f / (-1.0f * ret);
		return ret;
	}
	int getDelayKnobIndex(int delayKnobIndex) {
		int i = (int) round( params[DELAY_PARAMS + delayKnobIndex].value );// [ 0 ; (numDelays-1) ]
		if (i < 0) 
			i = 0;
		if (i >= numDelays) 
			i = numDelays - 1;
		return i;
	}
	float getDelayFraction(int delayKnobIndex) {// fraction of clock period
		// delayKnobIndex is 0 for master (not applicable), and 1 to 3 for sub-clocks
		return delayValues[getDelayKnobIndex(delayKnobIndex)];
	}	
	std::string getDelayFractionLabel(int delayKnobIndex) {
		// label for fraction of clock period 
		return delayLabelsClock[getDelayKnobIndex(delayKnobIndex)];
	}	
	std::string getDelayNoteLabel(int delayKnobIndex) {
		// label for fraction of clock period in notes (one period is a quarter note)
		return delayLabelsNote[getDelayKnobIndex(delayKnobIndex)];
	}	
	
	// Need to save
	int panelTheme = 0;
	int expansion = 0;
	bool running;
	bool displayDelayNoteMode = true;
	
	// No need to save
	float resetLight = 0.0f;
	static constexpr float BPM_PARAM_INIT_VALUE = 120.0f;// so that module constructor is coherent with widget initialization, since module created before widget
	int bpm;
	Clock clk[4];
	ClockDelay delay[4];
	int ratiosDoubled[4];
	int newRatiosDoubled[4];
	
	static constexpr float SWING_PARAM_INIT_VALUE = 0.0f;// so that module constructor is coherent with widget initialization, since module created before widget
	float swingVal[4];
	float swingLast[4];
	long swingInfo[4];// downward step counter when swing to be displayed, 0 when normal display

	static constexpr float DELAY_PARAM_INIT_VALUE = 0.0f;// so that module constructor is coherent with widget initialization, since module created before widget
	float delayVal[4];
	float delayLast[4];
	long delayInfo[4];// downward step counter when delay to be displayed, 0 when normal display
	
	SchmittTrigger resetTrigger;
	SchmittTrigger runTrigger;
	PulseGenerator resetPulse;
	PulseGenerator runPulse;
	

	Clocked() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		for (int i = 0; i < numRatios; i++) {
			ratioValuesDoubled[i] = (int) (ratioValues[i] * 2.0f + 0.5f);
		}
		for (int i = 0; i < 4; i++) {
			clk[i].setSync(i == 0 ? nullptr : &clk[0]);
		}
		onReset();
	}
	

	// widgets are not yet created when module is created (and when onReset() is called by constructor)
	// onReset() is also called when right-click initialization of module
	void onReset() override {
		running = false;
		bpm = (int)(BPM_PARAM_INIT_VALUE + 0.5f);
		for (int i = 0; i < 4; i++) {
			clk[i].reset();
			delay[i].reset();
			ratiosDoubled[i] = 0;
			newRatiosDoubled[i] = 0;
			swingVal[i] = SWING_PARAM_INIT_VALUE;
			swingLast[i] = swingVal[i];
			swingInfo[i] = 0l;
			delayVal[i] = DELAY_PARAM_INIT_VALUE;
			delayLast[i] = delayVal[i];
			delayInfo[i] = 0l;
		}
	}
	
	
	// widgets randomized before onRandomize() is called
	void onRandomize() override {
		onReset();
		for (int i = 0; i < 4; i++) {
			swingVal[i] = params[SWING_PARAMS + i].value;// redo this since SWING_PARAM_INIT_VALUE of onReset() not valid
			swingLast[i] = swingVal[i];
			delayVal[i] = params[DELAY_PARAMS + i].value;// redo this since DELAY_PARAM_INIT_VALUE of onReset() not valid
			delayLast[i] = delayVal[i];
		}
		bpm = getBeatsPerMinute();// redo this since BPM_PARAM_INIT_VALUE of onReset() not valid
	}

	
	json_t *toJson() override {
		json_t *rootJ = json_object();
		
		// panelTheme
		json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

		// expansion
		json_object_set_new(rootJ, "expansion", json_integer(expansion));

		// running
		json_object_set_new(rootJ, "running", json_boolean(running));
		
		// displayDelayNoteMode
		json_object_set_new(rootJ, "displayDelayNoteMode", json_boolean(displayDelayNoteMode));
		
		return rootJ;
	}


	// widgets loaded before this fromJson() is called
	void fromJson(json_t *rootJ) override {
		onReset();
		
		// panelTheme
		json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
		if (panelThemeJ)
			panelTheme = json_integer_value(panelThemeJ);

		// expansion
		json_t *expansionJ = json_object_get(rootJ, "expansion");
		if (expansionJ)
			expansion = json_integer_value(expansionJ);

		// running
		json_t *runningJ = json_object_get(rootJ, "running");
		if (runningJ)
			running = json_is_true(runningJ);

		// displayDelayNoteMode
		json_t *displayDelayNoteModeJ = json_object_get(rootJ, "displayDelayNoteMode");
		if (displayDelayNoteModeJ)
			displayDelayNoteMode = json_is_true(displayDelayNoteModeJ);

		for (int i = 0; i < 4; i++) {
			swingVal[i] = params[SWING_PARAMS + i].value;
			swingLast[i] = swingVal[i];
			delayVal[i] = params[DELAY_PARAMS + i].value;
			delayLast[i] = delayVal[i];
		}
		
		bpm = getBeatsPerMinute();
	}

	// Advances the module by 1 audio frame with duration 1.0 / engineGetSampleRate()
	void step() override {		
		double sampleRate = (double)engineGetSampleRate();
		double sampleTime = 1.0 / sampleRate;// do this here since engineGetSampleRate() returns float
		
		static const float swingInfoTime = 2.0f;// seconds
		long swingInfoInit = (long) (swingInfoTime * (float)sampleRate);
	
		static const float delayInfoTime = 3.0f;// seconds
		long delayInfoInit = (long) (delayInfoTime * (float)sampleRate);
		
		// Run button
		if (runTrigger.process(params[RUN_PARAM].value + inputs[RUN_INPUT].value)) {
			running = !running;
			// reset on any change of run state (will not relaunch if not running, thus clock railed low)
			for (int i = 0; i < 4; i++) {
				clk[i].reset();
				delay[i].reset();
			}
			runPulse.trigger(0.001f);
		}

		// Reset (has to be near to because sets steps to 0, and 0 not a real step (clock section will move to 1 before reaching outputs)
		if (resetTrigger.process(inputs[RESET_INPUT].value + params[RESET_PARAM].value)) {// TODO add sampleRate change as condition for reset
			resetLight = 1.0f;
			resetPulse.trigger(0.001f);
			for (int i = 0; i < 4; i++) {
				clk[i].reset();
				delay[i].reset();
			}
		}
		else
			resetLight -= (resetLight / lightLambda) * (float)sampleTime;	

		// BPM input and knob
		int newBpm = getBeatsPerMinute();
		if (newBpm != bpm) {
			double lengthStretchFactor = ((double)bpm) / ((double)newBpm);
			for (int i = 0; i < 4; i++) {
				clk[i].applyNewLength(lengthStretchFactor);
			}
			bpm = newBpm;// bpm was changed
		}

		// Ratio knobs
		bool syncRatios[4] = {false, false, false, false};// 0 index unused
		for (int i = 1; i < 4; i++) {
			newRatiosDoubled[i] = getRatioDoubled(i);
			if (newRatiosDoubled[i] != ratiosDoubled[i]) {
				syncRatios[i] = true;// 0 index not used, but loop must start at i = 0
			}
		}
		
		// Swing and delay changed (for swing info)
		for (int i = 0; i < 4; i++) {
			swingVal[i] = params[SWING_PARAMS + i].value;
			if (swingLast[i] != swingVal[i]) {
				swingInfo[i] = swingInfoInit;// trigger swing info on channel i
				delayInfo[i] = 0l;
				swingLast[i] = swingVal[i];
			}
			delayVal[i] = params[DELAY_PARAMS + i].value;
			if (delayLast[i] != delayVal[i]) {
				delayInfo[i] = delayInfoInit;// trigger delay info on channel i, use same init value as swing
				swingInfo[i] = 0l;
				delayLast[i] = delayVal[i];
			}
		}			

		
		
		//********** Clocks and Delays **********
		
		// Clocks
		if (running) {
			// See if clocks finished their prescribed number of iteratios of double periods (and syncWait for sub) or were forced reset
			//    and if so, recalc and restart them
			// Note: the 0.0d step does not consume a sample step when runnnig, it's used to force length (re)computation
			//     and will stay at 0.0d when a clock is inactive.
			
			// Master clock
			double masterLength = 120.0 / (double)bpm;// two periods
			if (clk[0].isReset()) {
				// See if ratio knobs changed (or unitinialized)
				for (int i = 1; i < 4; i++) {
					if (syncRatios[i]) {// always false for master
						clk[i].reset();// force reset (thus refresh) of that sub-clock
						ratiosDoubled[i] = newRatiosDoubled[i];
						syncRatios[i] = false;
					}
				}
				clk[0].setup(masterLength, 1, sampleTime);// must call setup before start
				clk[0].start();
			}
			
			// Sub clocks
			for (int i = 1; i < 4; i++) {
				if (clk[i].isReset()) {
					double length;
					int iterations;
					int ratioDoubled = ratiosDoubled[i];
					if (ratioDoubled < 0) { // if div 
						ratioDoubled *= -1;
						length = (masterLength * ((double)ratioDoubled)) / 2.0;
						iterations = 1l + (ratioDoubled % 2);		
						clk[i].setup(length, iterations, sampleTime);
					}
					else {// mult 
						length = (masterLength * 2.0) / ((double)ratioDoubled);
						iterations = ratioDoubled / (2l - (ratioDoubled % 2l));							
						clk[i].setup(length, iterations, sampleTime);
					}
					clk[i].start();
				}
			}
		}
			
		// Delays
		for (int i = 0; i < 4; i++) {
			float pulseWidth = params[PW_PARAMS + i].value;
			if (i < 3 && inputs[PW_INPUTS + i].active) {
				pulseWidth += (inputs[PW_INPUTS + i].value / 10.0f) - 0.5f;
				pulseWidth = clamp(pulseWidth, 0.0f, 1.0f);
			}
			float swingAmount = swingVal[i];
			if (i < 3 && inputs[SWING_INPUTS + i].active) {
				swingAmount += (inputs[SWING_INPUTS + i].value / 5.0f) - 1.0f;
				swingAmount = clamp(swingAmount, -1.0f, 1.0f);
			}
			delay[i].write(clk[i].isHigh(swingAmount, pulseWidth));
		}
		
		
		
		//********** Outputs and lights **********
		
		// Clock outputs
		float masterPeriod = 60.0f / (float)bpm;// result in seconds
		for (int i = 0; i < 4; i++) {
			long delaySamples = 0l;
			if (i > 0) 
				delaySamples = (long)(masterPeriod * getDelayFraction(i) * sampleRate / calcRatio(ratiosDoubled[i])) ;
			
			outputs[CLK_OUTPUTS + i].value = delay[i].read(delaySamples) ? 10.0f : 0.0f;
		}
		
		// Chaining outputs
		outputs[RESET_OUTPUT].value = (resetPulse.process((float)sampleTime) ? 10.0f : 0.0f);
		outputs[RUN_OUTPUT].value = (runPulse.process((float)sampleTime) ? 10.0f : 0.0f);
		outputs[BPM_OUTPUT].value =  (inputs[BPM_INPUT].active ? inputs[BPM_INPUT].value : ( (float)((bpm - bpmMin) * 10l) / ((float)bpmRange) ) );
			
		// Reset light
		lights[RESET_LIGHT].value =	resetLight;	
		
		// Run light
		lights[RUN_LIGHT].value = running;
		
		// Sync lights
		for (int i = 1; i < 4; i++) {
			lights[CLK_LIGHTS + i].value = (syncRatios[i] && running) ? 1.0f: 0.0f;
		}

		// Incr/decr all counters related to step()
		for (int i = 0; i < 4; i++) {
			clk[i].stepClock();
			if (swingInfo[i] > 0)
				swingInfo[i]--;
			if (delayInfo[i] > 0)
				delayInfo[i]--;
		}
	}
};


struct ClockedWidget : ModuleWidget {
	Clocked *module;
	DynamicSVGPanelEx *panel;
	int oldExpansion;
	IMPort* expPorts[5];

	struct RatioDisplayWidget : TransparentWidget {
		Clocked *module;
		int knobIndex;
		std::shared_ptr<Font> font;
		char displayStr[4];
		
		RatioDisplayWidget() {
			font = Font::load(assetPlugin(plugin, "res/fonts/Segment14.ttf"));
		}
		
		void draw(NVGcontext *vg) override {
			NVGcolor textColor = prepareDisplay(vg, &box);
			nvgFontFaceId(vg, font->handle);
			//nvgTextLetterSpacing(vg, 2.5);

			Vec textPos = Vec(6, 24);
			nvgFillColor(vg, nvgTransRGBA(textColor, 16));
			nvgText(vg, textPos.x, textPos.y, "~~~", NULL);
			nvgFillColor(vg, textColor);
			if (module->swingInfo[knobIndex] > 0)
			{
				float swValue = module->swingVal[knobIndex];
				int swInt = (int)round(swValue * 99.0f);
				snprintf(displayStr, 4, " %2u", (unsigned) abs(swInt));
				if (swInt < 0)
					displayStr[0] = '-';
				if (swInt > 0)
					displayStr[0] = '+';
			}
			else if (module->delayInfo[knobIndex] > 0)
			{
				if (module->displayDelayNoteMode)
					snprintf(displayStr, 4, "%s", (module->getDelayNoteLabel(knobIndex)).c_str());	
				else
					snprintf(displayStr, 4, "%s", (module->getDelayFractionLabel(knobIndex)).c_str());	
			}
			else {
				if (knobIndex > 0) {// ratio to display
					bool isDivision = false;
					int ratioDoubled = module->getRatioDoubled(knobIndex);
					if (ratioDoubled < 0) {
						ratioDoubled = -1 * ratioDoubled;
						isDivision = true;
					}
					if ( (ratioDoubled % 2) == 1 )
						snprintf(displayStr, 4, "%c,5", 0x30 + (char)(ratioDoubled / 2));
					else {
						snprintf(displayStr, 4, "X%2u", (unsigned)(ratioDoubled / 2));
						if (isDivision)
							displayStr[0] = '/';
					}
				}
				else {// BPM to display
					snprintf(displayStr, 4, "%3u", (unsigned) (fabs(module->getBeatsPerMinute()) + 0.5f));
				}
			}
			displayStr[3] = 0;// more safety
			nvgText(vg, textPos.x, textPos.y, displayStr, NULL);
		}
	};		
	
	struct PanelThemeItem : MenuItem {
		Clocked *module;
		int theme;
		void onAction(EventAction &e) override {
			module->panelTheme = theme;
		}
		void step() override {
			rightText = (module->panelTheme == theme) ? "✔" : "";
		}
	};
	struct ExpansionItem : MenuItem {
		Clocked *module;
		void onAction(EventAction &e) override {
			module->expansion = module->expansion == 1 ? 0 : 1;
		}
	};
	struct DelayDisplayNoteItem : MenuItem {
		Clocked *module;
		void onAction(EventAction &e) override {
			module->displayDelayNoteMode = !module->displayDelayNoteMode;
		}
	};
	Menu *createContextMenu() override {
		Menu *menu = ModuleWidget::createContextMenu();

		MenuLabel *spacerLabel = new MenuLabel();
		menu->addChild(spacerLabel);

		Clocked *module = dynamic_cast<Clocked*>(this->module);
		assert(module);

		MenuLabel *themeLabel = new MenuLabel();
		themeLabel->text = "Panel Theme";
		menu->addChild(themeLabel);

		PanelThemeItem *lightItem = new PanelThemeItem();
		lightItem->text = lightPanelID;// ImpromptuModular.hpp
		lightItem->module = module;
		lightItem->theme = 0;
		menu->addChild(lightItem);

		PanelThemeItem *darkItem = new PanelThemeItem();
		darkItem->text = darkPanelID;// ImpromptuModular.hpp
		darkItem->module = module;
		darkItem->theme = 1;
		menu->addChild(darkItem);

		menu->addChild(new MenuLabel());// empty line
		
		MenuLabel *settingsLabel = new MenuLabel();
		settingsLabel->text = "Settings";
		menu->addChild(settingsLabel);
		
		DelayDisplayNoteItem *ddnItem = MenuItem::create<DelayDisplayNoteItem>("Display Delay Values in Notes", CHECKMARK(module->displayDelayNoteMode));
		ddnItem->module = module;
		menu->addChild(ddnItem);

		menu->addChild(new MenuLabel());// empty line
		
		MenuLabel *expansionLabel = new MenuLabel();
		expansionLabel->text = "Expansion module";
		menu->addChild(expansionLabel);

		ExpansionItem *expItem = MenuItem::create<ExpansionItem>(expansionMenuLabel, CHECKMARK(module->expansion != 0));
		expItem->module = module;
		menu->addChild(expItem);

		return menu;
	}	
	
	void step() override {
		if(module->expansion != oldExpansion) {
			if (oldExpansion!= -1 && module->expansion == 0) {// if just removed expansion panel, disconnect wires to those jacks
				for (int i = 0; i < 5; i++)
					gRackWidget->wireContainer->removeAllWires(expPorts[i]);
			}
			oldExpansion = module->expansion;		
		}
		box.size = panel->box.size;
		Widget::step();
	}
	
	ClockedWidget(Clocked *module) : ModuleWidget(module) {
 		this->module = module;
		oldExpansion = -1;
		
		// Main panel from Inkscape
        panel = new DynamicSVGPanelEx();
        panel->mode = &module->panelTheme;
        panel->expansion = &module->expansion;
        panel->addPanel(SVG::load(assetPlugin(plugin, "res/light/Clocked.svg")));
        panel->addPanel(SVG::load(assetPlugin(plugin, "res/dark/Clocked_dark.svg")));
        panel->addPanelEx(SVG::load(assetPlugin(plugin, "res/light/ClockedExp.svg")));
        panel->addPanelEx(SVG::load(assetPlugin(plugin, "res/dark/ClockedExp_dark.svg")));
        box.size = panel->box.size;
        addChild(panel);		
		
		// Screws
		addChild(createDynamicScrew<IMScrew>(Vec(15, 0), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(15, 365), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x-30, 0), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x-30, 365), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x+30, 0), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x+30, 365), &module->panelTheme));


		static const int rowRuler0 = 50;//reset,run inputs, master knob and bpm display
		static const int rowRuler1 = rowRuler0 + 55;// reset,run switches
		//
		static const int rowRuler2 = rowRuler1 + 55;// clock 1
		static const int rowSpacingClks = 50;
		static const int rowRuler5 = rowRuler2 + rowSpacingClks * 2 + 55;// reset,run outputs, pw inputs
		
		
		static const int colRulerL = 18;// reset input and button, ratio knobs
		// First two rows and last row
		static const int colRulerSpacingT = 47;
		static const int colRulerT1 = colRulerL + colRulerSpacingT;// run input and button
		static const int colRulerT2 = colRulerT1 + colRulerSpacingT;// in and pwMaster inputs
		static const int colRulerT3 = colRulerT2 + colRulerSpacingT + 5;// swingMaster knob
		static const int colRulerT4 = colRulerT3 + colRulerSpacingT;// pwMaster knob
		static const int colRulerT5 = colRulerT4 + colRulerSpacingT;// clkMaster output
		// Three clock rows
		static const int colRulerM0 = colRulerL + 5;// ratio knobs
		static const int colRulerM1 = colRulerL + 60;// ratio displays
		static const int colRulerM2 = colRulerT3;// swingX knobs
		static const int colRulerM3 = colRulerT4;// pwX knobs
		static const int colRulerM4 = colRulerT5;// clkX outputs
		
		RatioDisplayWidget *displayRatios[4];
		
		// Row 0
		// Reset input
		addInput(createDynamicPort<IMPort>(Vec(colRulerL, rowRuler0), Port::INPUT, module, Clocked::RESET_INPUT, &module->panelTheme));
		// Run input
		addInput(createDynamicPort<IMPort>(Vec(colRulerT1, rowRuler0), Port::INPUT, module, Clocked::RUN_INPUT, &module->panelTheme));
		// In input
		addInput(createDynamicPort<IMPort>(Vec(colRulerT2, rowRuler0), Port::INPUT, module, Clocked::BPM_INPUT, &module->panelTheme));
		// Master BPM knob
		addParam(createDynamicParam<IMBigSnapKnob>(Vec(colRulerT3 + 1 + offsetIMBigKnob, rowRuler0 + offsetIMBigKnob), module, Clocked::RATIO_PARAMS + 0, (float)(module->bpmMin), (float)(module->bpmMax), Clocked::BPM_PARAM_INIT_VALUE, &module->panelTheme));		
		// BPM display
		displayRatios[0] = new RatioDisplayWidget();
		displayRatios[0]->box.pos = Vec(colRulerT4 + 11, rowRuler0 + vOffsetDisplay);
		displayRatios[0]->box.size = Vec(55, 30);// 3 characters
		displayRatios[0]->module = module;
		displayRatios[0]->knobIndex = 0;
		addChild(displayRatios[0]);
		
		// Row 1
		// Reset LED bezel and light
		addParam(ParamWidget::create<LEDBezel>(Vec(colRulerL + offsetLEDbezel, rowRuler1 + offsetLEDbezel), module, Clocked::RESET_PARAM, 0.0f, 1.0f, 0.0f));
		addChild(ModuleLightWidget::create<MuteLight<GreenLight>>(Vec(colRulerL + offsetLEDbezel + offsetLEDbezelLight, rowRuler1 + offsetLEDbezel + offsetLEDbezelLight), module, Clocked::RESET_LIGHT));
		// Run LED bezel and light
		addParam(ParamWidget::create<LEDBezel>(Vec(colRulerT1 + offsetLEDbezel, rowRuler1 + offsetLEDbezel), module, Clocked::RUN_PARAM, 0.0f, 1.0f, 0.0f));
		addChild(ModuleLightWidget::create<MuteLight<GreenLight>>(Vec(colRulerT1 + offsetLEDbezel + offsetLEDbezelLight, rowRuler1 + offsetLEDbezel + offsetLEDbezelLight), module, Clocked::RUN_LIGHT));
		// PW master input
		addInput(createDynamicPort<IMPort>(Vec(colRulerT2, rowRuler1), Port::INPUT, module, Clocked::PW_INPUTS + 0, &module->panelTheme));
		// Swing master knob
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerT3 + offsetIMSmallKnob, rowRuler1 + offsetIMSmallKnob), module, Clocked::SWING_PARAMS + 0, -1.0f, 1.0f, Clocked::SWING_PARAM_INIT_VALUE, &module->panelTheme));
		// PW master knob
		addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerT4 + offsetIMSmallKnob, rowRuler1 + offsetIMSmallKnob), module, Clocked::PW_PARAMS + 0, 0.0f, 1.0f, 0.5f, &module->panelTheme));
		// Clock master out
		addOutput(createDynamicPort<IMPort>(Vec(colRulerT5, rowRuler1), Port::OUTPUT, module, Clocked::CLK_OUTPUTS + 0, &module->panelTheme));
		
		
		// Row 2-4 (sub clocks)		
		for (int i = 0; i < 3; i++) {
			// Ratio1 knob
			addParam(createDynamicParam<IMBigSnapKnob>(Vec(colRulerM0 + offsetIMBigKnob, rowRuler2 + i * rowSpacingClks + offsetIMBigKnob), module, Clocked::RATIO_PARAMS + 1 + i, ((float)(module->numRatios - 1))*-1.0f, ((float)(module->numRatios - 1)), 0.0f, &module->panelTheme));		
			// Ratio display
			displayRatios[i + 1] = new RatioDisplayWidget();
			displayRatios[i + 1]->box.pos = Vec(colRulerM1, rowRuler2 + i * rowSpacingClks + vOffsetDisplay);
			displayRatios[i + 1]->box.size = Vec(55, 30);// 3 characters
			displayRatios[i + 1]->module = module;
			displayRatios[i + 1]->knobIndex = i + 1;
			addChild(displayRatios[i + 1]);
			// Sync light
			addChild(ModuleLightWidget::create<SmallLight<RedLight>>(Vec(colRulerM1 + 62, rowRuler2 + i * rowSpacingClks + 10), module, Clocked::CLK_LIGHTS + i + 1));		
			// Swing knobs
			addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerM2 + offsetIMSmallKnob, rowRuler2 + i * rowSpacingClks + offsetIMSmallKnob), module, Clocked::SWING_PARAMS + 1 + i, -1.0f, 1.0f, Clocked::SWING_PARAM_INIT_VALUE, &module->panelTheme));
			// PW knobs
			addParam(createDynamicParam<IMSmallKnob>(Vec(colRulerM3 + offsetIMSmallKnob, rowRuler2 + i * rowSpacingClks + offsetIMSmallKnob), module, Clocked::PW_PARAMS + 1 + i, 0.0f, 1.0f, 0.5f, &module->panelTheme));
			// Delay knobs
			addParam(createDynamicParam<IMSmallSnapKnob>(Vec(colRulerM4 + offsetIMSmallKnob, rowRuler2 + i * rowSpacingClks + offsetIMSmallKnob), module, Clocked::DELAY_PARAMS + 1 + i , Clocked::DELAY_PARAM_INIT_VALUE, ((float)(module->numDelays - 1)), 0.0f, &module->panelTheme));
		}

		// Last row
		// Reset out
		addOutput(createDynamicPort<IMPort>(Vec(colRulerL, rowRuler5), Port::OUTPUT, module, Clocked::RESET_OUTPUT, &module->panelTheme));
		// Run out
		addOutput(createDynamicPort<IMPort>(Vec(colRulerT1, rowRuler5), Port::OUTPUT, module, Clocked::RUN_OUTPUT, &module->panelTheme));
		// Out out
		addOutput(createDynamicPort<IMPort>(Vec(colRulerT2, rowRuler5), Port::OUTPUT, module, Clocked::BPM_OUTPUT, &module->panelTheme));
		// Sub-clock outputs
		addOutput(createDynamicPort<IMPort>(Vec(colRulerT3, rowRuler5), Port::OUTPUT, module, Clocked::CLK_OUTPUTS + 1, &module->panelTheme));	
		addOutput(createDynamicPort<IMPort>(Vec(colRulerT4, rowRuler5), Port::OUTPUT, module, Clocked::CLK_OUTPUTS + 2, &module->panelTheme));	
		addOutput(createDynamicPort<IMPort>(Vec(colRulerT5, rowRuler5), Port::OUTPUT, module, Clocked::CLK_OUTPUTS + 3, &module->panelTheme));	

		// Expansion module
		static const int rowRulerExpTop = 65;
		static const int rowSpacingExp = 60;
		static const int colRulerExp = 497 - 30 -150;// Clocked is (2+10)HP less than PS32
		addInput(expPorts[0] = createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 0), Port::INPUT, module, Clocked::PW_INPUTS + 1, &module->panelTheme));
		addInput(expPorts[1] = createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 1), Port::INPUT, module, Clocked::PW_INPUTS + 2, &module->panelTheme));
		addInput(expPorts[2] = createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 2), Port::INPUT, module, Clocked::SWING_INPUTS + 0, &module->panelTheme));
		addInput(expPorts[3] = createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 3), Port::INPUT, module, Clocked::SWING_INPUTS + 1, &module->panelTheme));
		addInput(expPorts[4] = createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 4), Port::INPUT, module, Clocked::SWING_INPUTS + 2, &module->panelTheme));

	}
};

Model *modelClocked = Model::create<Clocked, ClockedWidget>("Impromptu Modular", "Clocked", "CLK - Clocked", CLOCK_TAG);
