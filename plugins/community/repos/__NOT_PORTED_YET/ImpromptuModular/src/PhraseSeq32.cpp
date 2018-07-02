//***********************************************************************************************
//Multi-phrase 32 step sequencer module for VCV Rack by Marc Boulé
//
//Based on code from the Fundamental and AudibleInstruments plugins by Andrew Belt 
//and graphics from the Component Library by Wes Milholen 
//See ./LICENSE.txt for all licenses
//See ./res/fonts/ for font licenses
//
//Module inspired by the SA-100 Stepper Acid sequencer by Transistor Sounds Labs
//
//Acknowledgements: please see README.md
//***********************************************************************************************


#include "ImpromptuModular.hpp"
#include "dsp/digital.hpp"


struct PhraseSeq32 : Module {
	enum ParamIds {
		LEFT_PARAM,
		RIGHT_PARAM,
		RIGHT8_PARAM,// not used
		EDIT_PARAM,
		SEQUENCE_PARAM,
		RUN_PARAM,
		COPY_PARAM,
		PASTE_PARAM,
		RESET_PARAM,
		ENUMS(OCTAVE_PARAM, 7),
		GATE1_PARAM,
		GATE2_PARAM,
		SLIDE_BTN_PARAM,
		SLIDE_KNOB_PARAM,
		ATTACH_PARAM,
		AUTOSTEP_PARAM,
		ENUMS(KEY_PARAMS, 12),
		RUNMODE_PARAM,
		TRAN_ROT_PARAM,
		GATE1_KNOB_PARAM,
		GATE1_PROB_PARAM,
		TIE_PARAM,// Legato
		CPMODE_PARAM,
		ENUMS(STEP_PHRASE_PARAMS, 32),
		CONFIG_PARAM,
		// -- 0.6.4 ^^
		NUM_PARAMS
	};
	enum InputIds {
		WRITE_INPUT,
		CV_INPUT,
		RESET_INPUT,
		CLOCK_INPUT,
		LEFTCV_INPUT,
		RIGHTCV_INPUT,
		RUNCV_INPUT,
		SEQCV_INPUT,
		// -- 0.6.4 ^^
		MODECV_INPUT,
		GATE1CV_INPUT,
		GATE2CV_INPUT,
		TIEDCV_INPUT,
		SLIDECV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CVA_OUTPUT,
		GATE1A_OUTPUT,
		GATE2A_OUTPUT,
		CVB_OUTPUT,
		GATE1B_OUTPUT,
		GATE2B_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STEP_PHRASE_LIGHTS, 32 * 2),// room for GreenRed
		ENUMS(OCTAVE_LIGHTS, 7),// octaves 1 to 7
		ENUMS(KEY_LIGHTS, 12),
		RUN_LIGHT,
		RESET_LIGHT,
		GATE1_LIGHT,
		GATE2_LIGHT,
		SLIDE_LIGHT,
		ATTACH_LIGHT,
		GATE1_PROB_LIGHT,
		TIE_LIGHT,
		NUM_LIGHTS
	};
	
	enum DisplayStateIds {DISP_NORMAL, DISP_MODE, DISP_LENGTH, DISP_TRANSPOSE, DISP_ROTATE};
	enum AttributeBitMasks {ATT_MSK_GATE1 = 0x01, ATT_MSK_GATE1P = 0x02, ATT_MSK_GATE2 = 0x04, ATT_MSK_SLIDE = 0x08, ATT_MSK_TIED = 0x10};

	// Need to save
	int panelTheme = 0;
	int expansion = 0;
	bool running;
	int runModeSeq[32];
	int runModeSong; 
	//
	int sequence;
	int lengths[32];//1 to 32
	//
	int phrase[32];// This is the song (series of phases; a phrase is a patten number)
	int phrases;//1 to 32
	//
	float cv[32][32];// [-3.0 : 3.917]. First index is patten number, 2nd index is step
	int attributes[32][32];// First index is patten number, 2nd index is step (see enum AttributeBitMasks for details)
	//
	bool resetOnRun;
	bool attached;

	// No need to save
	float resetLight = 0.0f;
	int stepIndexEdit;
	int stepIndexRun;
	int phraseIndexEdit;
	int phraseIndexRun;
	long infoCopyPaste;// 0 when no info, positive downward step counter timer when copy, negative upward when paste
	unsigned long editingGate;// 0 when no edit gate, downward step counter timer when edit gate
	float editingGateCV;// no need to initialize, this is a companion to editingGate (output this only when editingGate > 0)
	int editingGateAttrib;// no need to initialize, this is a companion to editingGate (use this only when editingGate > 0)
	int editingChannel;// 0 means channel A, 1 means channel B. no need to initialize, this is a companion to editingGate
	int stepIndexRunHistory;// no need to initialize
	int phraseIndexRunHistory;// no need to initialize
	int displayState;
	unsigned long slideStepsRemain[2];// 0 when no slide under way, downward step counter when sliding
	float slideCVdelta[2];// no need to initialize, this is a companion to slideStepsRemain
	float cvCPbuffer[32];// copy paste buffer for CVs
	int attributesCPbuffer[32];// copy paste buffer for attributes
	int lengthCPbuffer;
	int modeCPbuffer;
	int countCP;// number of steps to paste (in case CPMODE_PARAM changes between copy and paste)
	int transposeOffset;// no need to initialize, this is companion to displayMode = DISP_TRANSPOSE
	int rotateOffset;// no need to initialize, this is companion to displayMode = DISP_ROTATE
	long clockIgnoreOnReset;
	const float clockIgnoreOnResetDuration = 0.001f;// disable clock on powerup and reset for 1 ms (so that the first step plays)
	unsigned long clockPeriod;// counts number of step() calls upward from last clock (reset after clock processed)
	long tiedWarning;// 0 when no warning, positive downward step counter timer when warning
	int sequenceKnob;// INT_MAX when knob not seen yet
	bool gate1RandomEnable[2];
	bool attachedChanB;
	long revertDisplay;
	
	static constexpr float CONFIG_PARAM_INIT_VALUE = 1.0f;// so that module constructor is coherent with widget initialization, since module created before widget
	int stepConfigLast;
	static constexpr float EDIT_PARAM_INIT_VALUE = 1.0f;// so that module constructor is coherent with widget initialization, since module created before widget
	bool editingSequence;
	bool editingSequenceLast;
	

	SchmittTrigger resetTrigger;
	SchmittTrigger leftTrigger;
	SchmittTrigger rightTrigger;
	SchmittTrigger runningTrigger;
	SchmittTrigger clockTrigger;
	SchmittTrigger octTriggers[7];
	SchmittTrigger octmTrigger;
	SchmittTrigger gate1Trigger;
	SchmittTrigger gate1ProbTrigger;
	SchmittTrigger gate2Trigger;
	SchmittTrigger slideTrigger;
	SchmittTrigger keyTriggers[12];
	SchmittTrigger writeTrigger;
	SchmittTrigger attachedTrigger;
	SchmittTrigger copyTrigger;
	SchmittTrigger pasteTrigger;
	SchmittTrigger modeTrigger;
	SchmittTrigger rotateTrigger;
	SchmittTrigger transposeTrigger;
	SchmittTrigger tiedTrigger;
	SchmittTrigger stepTriggers[32];
	

	inline bool getGate1(int seq, int step) {return (attributes[seq][step] & ATT_MSK_GATE1) != 0;}
	inline bool getGate2(int seq, int step) {return (attributes[seq][step] & ATT_MSK_GATE2) != 0;}
	inline bool getGate1P(int seq, int step) {return (attributes[seq][step] & ATT_MSK_GATE1P) != 0;}
	inline bool getTied(int seq, int step) {return (attributes[seq][step] & ATT_MSK_TIED) != 0;}
	inline bool isEditingSequence(void) {return params[EDIT_PARAM].value > 0.5f;}
	inline bool calcGate1RandomEnable(bool gate1P) {return (randomUniform() < (params[GATE1_KNOB_PARAM].value)) || !gate1P;}// randomUniform is [0.0, 1.0), see include/util/common.hpp
	
		
	PhraseSeq32() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {
		onReset();
	}

	
	// widgets are not yet created when module is created (and when onReset() is called by constructor)
	// onReset() is also called when right-click initialization of module
	void onReset() override {
		int stepConfig = 1;// 2x16
		if (CONFIG_PARAM_INIT_VALUE < 0.5f)// 1x32
			stepConfig = 2;
		stepConfigLast = stepConfig;
		running = false;
		runModeSong = MODE_FWD;
		stepIndexEdit = 0;
		phraseIndexEdit = 0;
		sequence = 0;
		phrases = 4;
		for (int i = 0; i < 32; i++) {
			for (int s = 0; s < 32; s++) {
				cv[i][s] = 0.0f;
				attributes[i][s] = ATT_MSK_GATE1;
			}
			runModeSeq[i] = MODE_FWD;
			phrase[i] = 0;
			lengths[i] = 16 * stepConfig;
			cvCPbuffer[i] = 0.0f;
			attributesCPbuffer[i] = ATT_MSK_GATE1;
		}
		initRun(stepConfig, true);
		lengthCPbuffer = 32;
		modeCPbuffer = MODE_FWD;
		countCP = 32;
		sequenceKnob = INT_MAX;
		editingGate = 0ul;
		infoCopyPaste = 0l;
		displayState = DISP_NORMAL;
		slideStepsRemain[0] = 0ul;
		slideStepsRemain[1] = 0ul;
		attached = true;
		clockPeriod = 0ul;
		tiedWarning = 0ul;
		attachedChanB = false;
		revertDisplay = 0l;
		editingSequence = EDIT_PARAM_INIT_VALUE > 0.5f;
		editingSequenceLast = editingSequence;
		resetOnRun = false;
	}
	
	
	// widgets randomized before onRandomize() is called
	void onRandomize() override {
		int stepConfig = 1;// 2x16
		if (params[CONFIG_PARAM].value < 0.5f)// 1x32
			stepConfig = 2;
		stepConfigLast = stepConfig;			
		running = false;
		runModeSong = randomu32() % 5;
		stepIndexEdit = 0;
		phraseIndexEdit = 0;
		sequence = randomu32() % 32;
		phrases = 1 + (randomu32() % 32);
		for (int i = 0; i < 32; i++) {
			for (int s = 0; s < 32; s++) {
				cv[i][s] = ((float)(randomu32() % 7)) + ((float)(randomu32() % 12)) / 12.0f - 3.0f;
				attributes[i][s] = randomu32() % 32;// 32 because 5 attributes
				if (getTied(i,s)) {
					attributes[i][s] = ATT_MSK_TIED;// clear other attributes if tied
					applyTiedStep(i, s, lengths[i]);
				}
			}
			runModeSeq[i] = randomu32() % 5;
			phrase[i] = randomu32() % 32;
			lengths[i] = 1 + (randomu32() % (16 * stepConfig));
			cvCPbuffer[i] = 0.0f;
			attributesCPbuffer[i] = ATT_MSK_GATE1;
		}
		initRun(stepConfig, true);
		lengthCPbuffer = 32;
		modeCPbuffer = MODE_FWD;
		countCP = 32;
		sequenceKnob = INT_MAX;
		editingGate = 0ul;
		infoCopyPaste = 0l;
		displayState = DISP_NORMAL;
		slideStepsRemain[0] = 0ul;
		slideStepsRemain[1] = 0ul;
		attached = true;
		clockPeriod = 0ul;
		tiedWarning = 0ul;
		attachedChanB = false;
		revertDisplay = 0l;
		editingSequence = isEditingSequence();
		editingSequenceLast = editingSequence;
		resetOnRun = false;
	}
	
	
	void initRun(int stepConfig, bool hard) {// run button activated or run edge in run input jack or edit mode toggled
		if (hard) {
			phraseIndexRun = (runModeSong == MODE_REV ? phrases - 1 : 0);
			if (editingSequence)
				stepIndexRun = (runModeSeq[sequence] == MODE_REV ? lengths[sequence] - 1 : 0);
			else
				stepIndexRun = (runModeSeq[phrase[phraseIndexRun]] == MODE_REV ? lengths[phrase[phraseIndexRun]] - 1 : 0);
		}
		gate1RandomEnable[0] = false;
		gate1RandomEnable[1] = false;
		if (editingSequence) {
			for (int i = 0; i < 2; i += stepConfig)
				gate1RandomEnable[i] = calcGate1RandomEnable(getGate1P(sequence, (i * 16) + stepIndexRun));
		}
		else {
			for (int i = 0; i < 2; i += stepConfig)
				gate1RandomEnable[i] = calcGate1RandomEnable(getGate1P(phrase[phraseIndexRun], (i * 16) + stepIndexRun));
		}
		clockIgnoreOnReset = (long) (clockIgnoreOnResetDuration * engineGetSampleRate());
	}
	
	
	json_t *toJson() override {
		json_t *rootJ = json_object();

		// panelTheme
		json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

		// expansion
		json_object_set_new(rootJ, "expansion", json_integer(expansion));

		// running
		json_object_set_new(rootJ, "running", json_boolean(running));
		
		// runModeSeq
		json_t *runModeSeqJ = json_array();
		for (int i = 0; i < 16; i++)
			json_array_insert_new(runModeSeqJ, i, json_integer(runModeSeq[i]));
		json_object_set_new(rootJ, "runModeSeq2", runModeSeqJ);// "2" appended so no break patches

		// runModeSong
		json_object_set_new(rootJ, "runModeSong", json_integer(runModeSong));

		// sequence
		json_object_set_new(rootJ, "sequence", json_integer(sequence));

		// lengths
		json_t *lengthsJ = json_array();
		for (int i = 0; i < 32; i++)
			json_array_insert_new(lengthsJ, i, json_integer(lengths[i]));
		json_object_set_new(rootJ, "lengths", lengthsJ);

		// phrase 
		json_t *phraseJ = json_array();
		for (int i = 0; i < 32; i++)
			json_array_insert_new(phraseJ, i, json_integer(phrase[i]));
		json_object_set_new(rootJ, "phrase", phraseJ);

		// phrases
		json_object_set_new(rootJ, "phrases", json_integer(phrases));

		// CV
		json_t *cvJ = json_array();
		for (int i = 0; i < 32; i++)
			for (int s = 0; s < 32; s++) {
				json_array_insert_new(cvJ, s + (i * 32), json_real(cv[i][s]));
			}
		json_object_set_new(rootJ, "cv", cvJ);

		// attributes
		json_t *attributesJ = json_array();
		for (int i = 0; i < 32; i++)
			for (int s = 0; s < 32; s++) {
				json_array_insert_new(attributesJ, s + (i * 32), json_integer(attributes[i][s]));
			}
		json_object_set_new(rootJ, "attributes", attributesJ);

		// attached
		json_object_set_new(rootJ, "attached", json_boolean(attached));

		// resetOnRun
		json_object_set_new(rootJ, "resetOnRun", json_boolean(resetOnRun));
		
		return rootJ;
	}

	
	// widgets loaded before this fromJson() is called
	void fromJson(json_t *rootJ) override {
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

		// runModeSeq
		json_t *runModeSeqJ = json_object_get(rootJ, "runModeSeq2");// "2" appended so no break patches
		if (runModeSeqJ) {
			for (int i = 0; i < 16; i++)
			{
				json_t *runModeSeqArrayJ = json_array_get(runModeSeqJ, i);
				if (runModeSeqArrayJ)
					runModeSeq[i] = json_integer_value(runModeSeqArrayJ);
			}			
		}		
		
		// runModeSong
		json_t *runModeSongJ = json_object_get(rootJ, "runModeSong");
		if (runModeSongJ)
			runModeSong = json_integer_value(runModeSongJ);
		
		// sequence
		json_t *sequenceJ = json_object_get(rootJ, "sequence");
		if (sequenceJ)
			sequence = json_integer_value(sequenceJ);
		
		// lengths
		json_t *lengthsJ = json_object_get(rootJ, "lengths");
		if (lengthsJ)
			for (int i = 0; i < 32; i++)
			{
				json_t *lengthsArrayJ = json_array_get(lengthsJ, i);
				if (lengthsArrayJ)
					lengths[i] = json_integer_value(lengthsArrayJ);
			}
			
		// phrase
		json_t *phraseJ = json_object_get(rootJ, "phrase");
		if (phraseJ)
			for (int i = 0; i < 32; i++)
			{
				json_t *phraseArrayJ = json_array_get(phraseJ, i);
				if (phraseArrayJ)
					phrase[i] = json_integer_value(phraseArrayJ);
			}
		
		// phrases
		json_t *phrasesJ = json_object_get(rootJ, "phrases");
		if (phrasesJ)
			phrases = json_integer_value(phrasesJ);
		
		// CV
		json_t *cvJ = json_object_get(rootJ, "cv");
		if (cvJ) {
			for (int i = 0; i < 32; i++)
				for (int s = 0; s < 32; s++) {
					json_t *cvArrayJ = json_array_get(cvJ, s + (i * 32));
					if (cvArrayJ)
						cv[i][s] = json_real_value(cvArrayJ);
				}
		}
		
		// attributes
		json_t *attributesJ = json_object_get(rootJ, "attributes");
		if (attributesJ) {
			for (int i = 0; i < 32; i++)
				for (int s = 0; s < 32; s++) {
					json_t *attributesArrayJ = json_array_get(attributesJ, s + (i * 32));
					if (attributesArrayJ)
						attributes[i][s] = json_integer_value(attributesArrayJ);
				}
		}
		
		// attached
		json_t *attachedJ = json_object_get(rootJ, "attached");
		if (attachedJ)
			attached = json_is_true(attachedJ);
		
		// resetOnRun
		json_t *resetOnRunJ = json_object_get(rootJ, "resetOnRun");
		if (resetOnRunJ)
			resetOnRun = json_is_true(resetOnRunJ);

		// Initialize dependants after everything loaded (widgets already loaded when reach here)
		int stepConfig = 1;// 2x16
		if (params[CONFIG_PARAM].value < 0.5f)// 1x32
			stepConfig = 2;
		stepConfigLast = stepConfig;			
		initRun(stepConfig, true);
		sequenceKnob = INT_MAX;
		editingSequence = isEditingSequence();
		editingSequenceLast = editingSequence;
	}

	void rotateSeq(int seqNum, bool directionRight, int seqLength, bool chanB_16) {
		// set chanB_16 to false to rotate chan A in 2x16 config (length will be <= 16) or single chan in 1x32 config (length will be <= 32)
		// set chanB_16 to true  to rotate chan B in 2x16 config (length must be <= 16)
		float rotCV;
		int rotAttributes;
		int iStart = chanB_16 ? 16 : 0;
		int iEnd = iStart + seqLength - 1;
		int iRot = iStart;
		int iDelta = 1;
		if (directionRight) {
			iRot = iEnd;
			iDelta = -1;
		}
		rotCV = cv[seqNum][iRot];
		rotAttributes = attributes[seqNum][iRot];
		for ( ; ; iRot += iDelta) {
			if (iDelta == 1 && iRot >= iEnd) break;
			if (iDelta == -1 && iRot <= iStart) break;
			cv[seqNum][iRot] = cv[seqNum][iRot + iDelta];
			attributes[seqNum][iRot] = attributes[seqNum][iRot + iDelta];
		}
		cv[seqNum][iRot] = rotCV;
		attributes[seqNum][iRot] = rotAttributes;
	}
	
	// Advances the module by 1 audio frame with duration 1.0 / engineGetSampleRate()
	void step() override {
		static const float gateTime = 0.4f;// seconds
		static const float copyPasteInfoTime = 0.5f;// seconds
		static const float revertDisplayTime = 0.7f;// seconds
		static const float tiedWarningTime = 0.7f;// seconds
		long tiedWarningInit = (long) (tiedWarningTime * engineGetSampleRate());
		
		
		//********** Buttons, knobs, switches and inputs **********
		
		// Notes: 
		// * a tied step's attributes can not be modified by any of the following: 
		//   write input, oct and keyboard buttons, gate1, gate1Prob, gate2 and slide buttons
		//   however, paste, transpose, rotate obviously can.
		// * Whenever cv[][] is modified or tied[] is activated for a step, call applyTiedStep(sequence,stepIndexEdit,steps)
		

		// Config switch
		int stepConfig = 1;// 2x16
		if (params[CONFIG_PARAM].value < 0.5f)// 1x32
			stepConfig = 2;
		// Config: set lengths to their new max when move switch
		if (stepConfigLast != stepConfig) {
			for (int i = 0; i < 32; i++)
				lengths[i] = 16 * stepConfig;
			attachedChanB = false;
			stepConfigLast = stepConfig;
		}

		// Edit mode
		editingSequence = isEditingSequence();// true = editing sequence, false = editing song
		if (editingSequenceLast != editingSequence) {
			if (running)
				initRun(stepConfig, true);
			displayState = DISP_NORMAL;
			editingSequenceLast = editingSequence;
		}
		
		// Seq CV input
		if (inputs[SEQCV_INPUT].active) {
			sequence = (int) clamp( round(inputs[SEQCV_INPUT].value * (32.0f - 1.0f) / 10.0f), 0.0f, (32.0f - 1.0f) );
			//if (stepIndexEdit >= lengths[sequence])// Commented for full edit capabilities
				//stepIndexEdit = lengths[sequence] - 1;// Commented for full edit capabilities
		}
		// Mode CV input
		if (inputs[MODECV_INPUT].active) {
			if (editingSequence)
				runModeSeq[sequence] = (int) clamp( round(inputs[MODECV_INPUT].value * 4.0f / 10.0f), 0.0f, 4.0f );
		}
		
		// Run button
		if (runningTrigger.process(params[RUN_PARAM].value + inputs[RUNCV_INPUT].value)) {
			running = !running;
			if (running)
				initRun(stepConfig, resetOnRun);
			displayState = DISP_NORMAL;
		}

		// Attach button
		if (attachedTrigger.process(params[ATTACH_PARAM].value)) {
			if (running) {
				attached = !attached;
				if (attached && editingSequence && stepConfig == 1 ) 
					attachedChanB = stepIndexEdit >= 16;
			}
			displayState = DISP_NORMAL;			
		}
		if (running && attached) {
			if (editingSequence)
				stepIndexEdit = stepIndexRun + ((attachedChanB && stepConfig == 1) ? 16 : 0);
			else
				phraseIndexEdit = phraseIndexRun;
		}
		
		// Copy button
		if (copyTrigger.process(params[COPY_PARAM].value)) {
			if (editingSequence) {
				infoCopyPaste = (long) (copyPasteInfoTime * engineGetSampleRate());
				//CPinfo must be set to 0 for copy/paste all, and 0x1ii for copy/paste 4 at pos ii, 0x2ii for copy/paste 8 at 0xii
				int sStart = stepIndexEdit;
				int sCount = 32;
				if (params[CPMODE_PARAM].value > 1.5f)// all
					sStart = 0;
				else if (params[CPMODE_PARAM].value < 0.5f)// 4
					sCount = 4;
				else// 8
					sCount = 8;
				countCP = sCount;
				for (int i = 0, s = sStart; i < countCP; i++, s++) {
					if (s >= 32) s = 0;
					cvCPbuffer[i] = cv[sequence][s];
					attributesCPbuffer[i] = attributes[sequence][s];
					if ((--sCount) <= 0)
						break;
				}
				lengthCPbuffer = lengths[sequence];
				modeCPbuffer = runModeSeq[sequence];
			}
			displayState = DISP_NORMAL;
		}
		// Paste button
		if (pasteTrigger.process(params[PASTE_PARAM].value)) {
			if (editingSequence) {
				infoCopyPaste = (long) (-1 * copyPasteInfoTime * engineGetSampleRate());
				int sStart = ((countCP == 32) ? 0 : stepIndexEdit);
				int sCount = countCP;
				for (int i = 0, s = sStart; i < countCP; i++, s++) {
					if (s >= 32) break;
					cv[sequence][s] = cvCPbuffer[i];
					attributes[sequence][s] = attributesCPbuffer[i];
					if ((--sCount) <= 0)
						break;
				}	
				if (params[CPMODE_PARAM].value > 1.5f) {// all
					lengths[sequence] = lengthCPbuffer;
					if (lengths[sequence] > 16 * stepConfig)
						lengths[sequence] = 16 * stepConfig;
					runModeSeq[sequence] = modeCPbuffer;
				}
			}
			displayState = DISP_NORMAL;
		}

		// Write input (must be before Left and Right in case route gate simultaneously to Right and Write for example)
		//  (write must be to correct step)
		bool writeTrig = writeTrigger.process(inputs[WRITE_INPUT].value);
		if (writeTrig) {
			if (editingSequence) {
				if (getTied(sequence,stepIndexEdit) & !inputs[TIEDCV_INPUT].active)
					tiedWarning = tiedWarningInit;
				else {			
					cv[sequence][stepIndexEdit] = inputs[CV_INPUT].value;
					applyTiedStep(sequence, stepIndexEdit, ((stepIndexEdit >= 16 && stepConfig == 1) ? 16 : 0) + lengths[sequence]);
					// Extra CVs from expansion panel:
					if (inputs[TIEDCV_INPUT].active)
						attributes[sequence][stepIndexEdit] = (inputs[TIEDCV_INPUT].value > 1.0f) ? (attributes[sequence][stepIndexEdit] | ATT_MSK_TIED) : (attributes[sequence][stepIndexEdit] & ~ATT_MSK_TIED);
					if (getTied(sequence, stepIndexEdit))
						attributes[sequence][stepIndexEdit] = ATT_MSK_TIED;// clear other attributes if tied
					else {
						if (inputs[GATE1CV_INPUT].active)
							attributes[sequence][stepIndexEdit] = (inputs[GATE1CV_INPUT].value > 1.0f) ? (attributes[sequence][stepIndexEdit] | ATT_MSK_GATE1) : (attributes[sequence][stepIndexEdit] & ~ATT_MSK_GATE1);
						if (inputs[GATE2CV_INPUT].active)
							attributes[sequence][stepIndexEdit] = (inputs[GATE2CV_INPUT].value > 1.0f) ? (attributes[sequence][stepIndexEdit] | ATT_MSK_GATE2) : (attributes[sequence][stepIndexEdit] & ~ATT_MSK_GATE2);
						if (inputs[SLIDECV_INPUT].active)
							attributes[sequence][stepIndexEdit] = (inputs[SLIDECV_INPUT].value > 1.0f) ? (attributes[sequence][stepIndexEdit] | ATT_MSK_SLIDE) : (attributes[sequence][stepIndexEdit] & ~ATT_MSK_SLIDE);
					}
					editingGate = (unsigned long) (gateTime * engineGetSampleRate());
					editingGateCV = cv[sequence][stepIndexEdit];
					editingGateAttrib = attributes[sequence][stepIndexEdit];
					editingChannel = (stepIndexEdit >= 16 * stepConfig) ? 1 : 0;
					// Autostep (after grab all active inputs)
					if (params[AUTOSTEP_PARAM].value > 0.5f) {
						stepIndexEdit += 1;
						//if (stepConfig == 1) { // Commented for full edit capabilities (whole if)
						//	if (stepIndexEdit >= lengths[sequence] && stepIndexEdit < 16)// if past length in chan A
						//		stepIndexEdit = 16;
						//	else if (stepIndexEdit >= 16 + lengths[sequence])// if past length in chan B (including >=32)
						//		stepIndexEdit = 0;
						//}
						//else // Commented for full edit capabilities
							if (stepIndexEdit >= 32)//lengths[sequence])// Commented for full edit capabilities
								stepIndexEdit = 0;						
					}
				}
			}
			displayState = DISP_NORMAL;
		}
		// Left and right CV inputs
		int delta = 0;
		if (leftTrigger.process(inputs[LEFTCV_INPUT].value)) { 
			delta = -1;
			if (displayState != DISP_LENGTH)
				displayState = DISP_NORMAL;
		}
		if (rightTrigger.process(inputs[RIGHTCV_INPUT].value)) {
			delta = +1;
			if (displayState != DISP_LENGTH)
				displayState = DISP_NORMAL;
		}
		if (delta != 0) {
			if (displayState == DISP_LENGTH) {
				if (editingSequence) {
					lengths[sequence] += delta;
					if (lengths[sequence] > (16 * stepConfig)) lengths[sequence] = (16 * stepConfig);
					if (lengths[sequence] < 1 ) lengths[sequence] = 1;
					lengths[sequence] = ((lengths[sequence] - 1) % (16 * stepConfig)) + 1;
					//if ( (stepIndexEdit % (16 * stepConfig)) >= lengths[sequence])// Commented for full edit capabilities
						//stepIndexEdit = (lengths[sequence] - 1) + 16 * (stepIndexEdit / (16 * stepConfig));// Commented for full edit capabilities
				}
				else {
					phrases += delta;
					if (phrases > 32) phrases = 32;
					if (phrases < 1 ) phrases = 1;
					//if (phraseIndexEdit >= phrases) phraseIndexEdit = phrases - 1;// Commented for full edit capabilities
				}
			}
			else {
				if (!running || !attached) {// don't move heads when attach and running
					if (editingSequence) {
						stepIndexEdit += delta;
						if (stepIndexEdit < 0)
							stepIndexEdit = ((stepConfig == 1) ? 16 : 0) + lengths[sequence] - 1;
						//if (stepConfig == 1) {// Commented for full edit capabilities (whole if)
						//	if (stepIndexEdit >= lengths[sequence] && stepIndexEdit < 16)// if past length in chan A
						//		stepIndexEdit = delta > 0 ? 16 : lengths[sequence] - 1;
						//	else if (stepIndexEdit >= 16 + lengths[sequence])// if past length in chan B (including >=32)
						//		stepIndexEdit = delta > 0 ? 0 : 16 + lengths[sequence] - 1;
						//}
						//else// Commented for full edit capabilities
							if (stepIndexEdit >= 32)//lengths[sequence]) // Commented for full edit capabilities
								stepIndexEdit = 0;
						if (!getTied(sequence,stepIndexEdit)) {// play if non-tied step
							if (!writeTrig) {// in case autostep when simultaneous writeCV and stepCV (keep what was done in Write Input block above)
								editingGate = (unsigned long) (gateTime * engineGetSampleRate());
								editingGateCV = cv[sequence][stepIndexEdit];
								editingGateAttrib = attributes[sequence][stepIndexEdit];
								editingChannel = (stepIndexEdit >= 16 * stepConfig) ? 1 : 0;
							}
						}
					}
					else
						phraseIndexEdit = moveIndex(phraseIndexEdit, phraseIndexEdit + delta, 32);//phrases);// Commented for full edit capabilities
				}
			}
		}

		// Step button presses
		int stepPressed = -1;
		for (int i = 0; i < 32; i++) {
			if (stepTriggers[i].process(params[STEP_PHRASE_PARAMS + i].value))
				stepPressed = i;
		}
		if (stepPressed != -1) {
			if (displayState == DISP_LENGTH) {
				if (editingSequence) {
					lengths[sequence] = (stepPressed % (16 * stepConfig)) + 1;
					//if ( (stepIndexEdit % (16 * stepConfig)) >= lengths[sequence])// Commented for full edit capabilities
						//stepIndexEdit = (lengths[sequence] - 1) + 16 * (stepIndexEdit / (16 * stepConfig));// Commented for full edit capabilities
				}
				else {
					phrases = stepPressed + 1;
					//if (phraseIndexEdit >= phrases) phraseIndexEdit = phrases - 1;// Commented for full edit capabilities
				}
				revertDisplay = (long) (revertDisplayTime * engineGetSampleRate());
			}
			else {
				if (!running || !attached) {// not running or detached
					if (editingSequence) {
						stepIndexEdit = stepPressed;
						//if ( (stepIndexEdit % (16 * stepConfig)) >= lengths[sequence])// Commented for full edit capabilities
							//stepIndexEdit = (lengths[sequence] - 1) + 16 * (stepIndexEdit / (16 * stepConfig));// Commented for full edit capabilities
						if (!getTied(sequence,stepIndexEdit)) {// play if non-tied step
							editingGate = (unsigned long) (gateTime * engineGetSampleRate());
							editingGateCV = cv[sequence][stepIndexEdit];
							editingGateAttrib = attributes[sequence][stepIndexEdit];
							editingChannel = (stepIndexEdit >= 16 * stepConfig) ? 1 : 0;
						}
					}
					else {
						phraseIndexEdit = stepPressed;
						//if (phraseIndexEdit >= phrases)// Commented for full edit capabilities
							//phraseIndexEdit = phrases - 1;// Commented for full edit capabilities
					}
				}
				else {// attached and running
					if (editingSequence) {
						if ((stepPressed < 16) && attachedChanB)
							attachedChanB = false;
						if ((stepPressed >= 16) && !attachedChanB)
							attachedChanB = true;					
					}
				}
				displayState = DISP_NORMAL;
			}
		} 
		
		// Mode/Length button
		if (modeTrigger.process(params[RUNMODE_PARAM].value)) {
			if (displayState == DISP_NORMAL || displayState == DISP_TRANSPOSE || displayState == DISP_ROTATE)
				displayState = DISP_LENGTH;
			else if (displayState == DISP_LENGTH)
				displayState = DISP_MODE;
			else
				displayState = DISP_NORMAL;
		}
		
		// Transpose/Rotate button
		if (transposeTrigger.process(params[TRAN_ROT_PARAM].value)) {
			if (editingSequence) {
				if (displayState == DISP_NORMAL || displayState == DISP_MODE || displayState == DISP_LENGTH) {
					displayState = DISP_TRANSPOSE;
					transposeOffset = 0;
				}
				else if (displayState == DISP_TRANSPOSE) {
					displayState = DISP_ROTATE;
					rotateOffset = 0;
				}
				else 
					displayState = DISP_NORMAL;
			}
		}			
		
		// Sequence knob  
		int newSequenceKnob = (int)roundf(params[SEQUENCE_PARAM].value*7.0f);
		if (sequenceKnob == INT_MAX)
			sequenceKnob = newSequenceKnob;
		int deltaKnob = newSequenceKnob - sequenceKnob;
		if (deltaKnob != 0) {
			if (abs(deltaKnob) <= 3) {// avoid discontinuous step (initialize for example)
				if (displayState == DISP_MODE) {
					if (editingSequence) {
						if (!inputs[MODECV_INPUT].active) {
							runModeSeq[sequence] += deltaKnob;
							if (runModeSeq[sequence] < 0) runModeSeq[sequence] = 0;
							if (runModeSeq[sequence] > 4) runModeSeq[sequence] = 4;
						}
					}
					else {
						runModeSong += deltaKnob;
						if (runModeSong < 0) runModeSong = 0;
						if (runModeSong > 4) runModeSong = 4;
					}
				}
				else if (displayState == DISP_LENGTH) {
					if (editingSequence) {
						lengths[sequence] += deltaKnob;
						if (lengths[sequence] > (16 * stepConfig)) lengths[sequence] = (16 * stepConfig);
						if (lengths[sequence] < 1 ) lengths[sequence] = 1;
						//if ( (stepIndexEdit % (16 * stepConfig)) >= lengths[sequence])// Commented for full edit capabilities
							//stepIndexEdit = (lengths[sequence] - 1) + 16 * (stepIndexEdit / (16 * stepConfig));// Commented for full edit capabilities
					}
					else {
						phrases += deltaKnob;
						if (phrases > 32) phrases = 32;
						if (phrases < 1 ) phrases = 1;
						//if (phraseIndexEdit >= phrases) phraseIndexEdit = phrases - 1;// Commented for full edit capabilities
					}
				}
				else if (displayState == DISP_TRANSPOSE) {
					if (editingSequence) {
						transposeOffset += deltaKnob;
						if (transposeOffset > 99) transposeOffset = 99;
						if (transposeOffset < -99) transposeOffset = -99;						
						// Tranpose by this number of semi-tones: deltaKnob
						float transposeOffsetCV = ((float)(deltaKnob))/12.0f;
						if (stepConfig == 1){ // 2x16 (transpose only the 16 steps corresponding to row where stepIndexEdit is located)
							int offset = stepIndexEdit < 16 ? 0 : 16;
							for (int s = offset; s < offset + 16; s++) 
								cv[sequence][s] += transposeOffsetCV;
						}
						else { // 1x32 (transpose all 32 steps)
							for (int s = 0; s < 32; s++) 
								cv[sequence][s] += transposeOffsetCV;
						}
					}
				}
				else if (displayState == DISP_ROTATE) {
					if (editingSequence) {
						rotateOffset += deltaKnob;
						if (rotateOffset > 99) rotateOffset = 99;
						if (rotateOffset < -99) rotateOffset = -99;	
						if (deltaKnob > 0 && deltaKnob < 99) {// Rotate right, 99 is safety
							for (int i = deltaKnob; i > 0; i--)
								rotateSeq(sequence, true, lengths[sequence], stepConfig == 1 && stepIndexEdit >= 16);
						}
						if (deltaKnob < 0 && deltaKnob > -99) {// Rotate left, 99 is safety
							for (int i = deltaKnob; i < 0; i++)
								rotateSeq(sequence, false, lengths[sequence], stepConfig == 1 && stepIndexEdit >= 16);
						}
					}						
				}
				else {// DISP_NORMAL
					if (editingSequence) {
						if (!inputs[SEQCV_INPUT].active) {
							sequence += deltaKnob;
							if (sequence < 0) sequence = 0;
							if (sequence >= 32) sequence = (32 - 1);
							//if (stepConfig == 1) {// Commented for full edit capabilities (whole if/else)
							//	if (stepIndexEdit >= 16 && (stepIndexEdit - 16) >= lengths[sequence])
							//		stepIndexEdit = 16 + lengths[sequence] - 1;
							//	else if (stepIndexEdit < 16 && (stepIndexEdit) >= lengths[sequence])
							//		stepIndexEdit = lengths[sequence] - 1;
							//}
							//else {
							//	if (stepIndexEdit >= lengths[sequence]) 
							//		stepIndexEdit = lengths[sequence] - 1;
							//}
						}
					}
					else {
						phrase[phraseIndexEdit] += deltaKnob;
						if (phrase[phraseIndexEdit] < 0) phrase[phraseIndexEdit] = 0;
						if (phrase[phraseIndexEdit] >= 32) phrase[phraseIndexEdit] = (32 - 1);				
					}
				}
			}
			sequenceKnob = newSequenceKnob;
		}	
		
		// Octave buttons
		int newOct = -1;
		for (int i = 0; i < 7; i++) {
			if (octTriggers[i].process(params[OCTAVE_PARAM + i].value)) {
				newOct = 6 - i;
				displayState = DISP_NORMAL;
			}
		}
		if (newOct >= 0 && newOct <= 6) {
			if (editingSequence) {
				if (getTied(sequence,stepIndexEdit))
					tiedWarning = tiedWarningInit;
				else {			
					float newCV = cv[sequence][stepIndexEdit] + 10.0f;//to properly handle negative note voltages
					newCV = newCV - floor(newCV) + (float) (newOct - 3);
					if (newCV >= -3.0f && newCV < 4.0f) {
						cv[sequence][stepIndexEdit] = newCV;
						applyTiedStep(sequence, stepIndexEdit, ((stepIndexEdit >= 16 && stepConfig == 1) ? 16 : 0) + lengths[sequence]);
					}
					editingGate = (unsigned long) (gateTime * engineGetSampleRate());
					editingGateCV = cv[sequence][stepIndexEdit];
					editingGateAttrib = attributes[sequence][stepIndexEdit];
					editingChannel = (stepIndexEdit >= 16 * stepConfig) ? 1 : 0;
				}
			}
		}		
		
		// Keyboard buttons
		for (int i = 0; i < 12; i++) {
			if (keyTriggers[i].process(params[KEY_PARAMS + i].value)) {
				if (editingSequence) {
					if (getTied(sequence,stepIndexEdit))
						tiedWarning = tiedWarningInit;
					else {			
						cv[sequence][stepIndexEdit] = floor(cv[sequence][stepIndexEdit]) + ((float) i) / 12.0f;
						applyTiedStep(sequence, stepIndexEdit, ((stepIndexEdit >= 16 && stepConfig == 1) ? 16 : 0) + lengths[sequence]);
						editingGate = (unsigned long) (gateTime * engineGetSampleRate());
						editingGateCV = cv[sequence][stepIndexEdit];
						editingGateAttrib = attributes[sequence][stepIndexEdit];
						editingChannel = (stepIndexEdit >= 16 * stepConfig) ? 1 : 0;
					}						
				}
				displayState = DISP_NORMAL;
			}
		}
				
		// Gate1, Gate1Prob, Gate2, Slide and Tied buttons
		if (gate1Trigger.process(params[GATE1_PARAM].value)) {
			if (editingSequence) {
				if (getTied(sequence,stepIndexEdit))
					tiedWarning = tiedWarningInit;
				else
					attributes[sequence][stepIndexEdit] ^= ATT_MSK_GATE1;
			}
			displayState = DISP_NORMAL;
		}		
		if (gate1ProbTrigger.process(params[GATE1_PROB_PARAM].value)) {
			if (editingSequence) {
				if (getTied(sequence,stepIndexEdit))
					tiedWarning = tiedWarningInit;
				else
					attributes[sequence][stepIndexEdit] ^= ATT_MSK_GATE1P;
			}
			displayState = DISP_NORMAL;
		}		
		if (gate2Trigger.process(params[GATE2_PARAM].value)) {
			if (editingSequence) {
				if (getTied(sequence,stepIndexEdit))
					tiedWarning = tiedWarningInit;
				else
					attributes[sequence][stepIndexEdit] ^= ATT_MSK_GATE2;
			}
			displayState = DISP_NORMAL;
		}		
		if (slideTrigger.process(params[SLIDE_BTN_PARAM].value)) {
			if (editingSequence) {
				if (getTied(sequence,stepIndexEdit))
					tiedWarning = tiedWarningInit;
				else
					attributes[sequence][stepIndexEdit] ^= ATT_MSK_SLIDE;
			}
			displayState = DISP_NORMAL;
		}		
		if (tiedTrigger.process(params[TIE_PARAM].value)) {
			if (editingSequence) {
				attributes[sequence][stepIndexEdit] ^= ATT_MSK_TIED;
				if (getTied(sequence,stepIndexEdit)) {
					attributes[sequence][stepIndexEdit] = ATT_MSK_TIED;// clear other attributes if tied
					applyTiedStep(sequence, stepIndexEdit, ((stepIndexEdit >= 16 && stepConfig == 1) ? 16 : 0) + lengths[sequence]);
				}
				else
					attributes[sequence][stepIndexEdit] |= (ATT_MSK_GATE1 | ATT_MSK_GATE2);
			}
			displayState = DISP_NORMAL;
		}		
		
		
		//********** Clock and reset **********
		
		// Clock
		if (clockTrigger.process(inputs[CLOCK_INPUT].value)) {
			if (running && clockIgnoreOnReset == 0l) {
				float slideFromCV[2] = {0.0f, 0.0f};
				float slideToCV[2] = {0.0f, 0.0f};
				if (editingSequence) {
					for (int i = 0; i < 2; i += stepConfig)
						slideFromCV[i] = cv[sequence][(i * 16) + stepIndexRun];
					moveIndexRunMode(&stepIndexRun, lengths[sequence], runModeSeq[sequence], &stepIndexRunHistory);
					for (int i = 0; i < 2; i += stepConfig) {
						slideToCV[i] = cv[sequence][(i * 16) + stepIndexRun];
						gate1RandomEnable[i] = calcGate1RandomEnable(getGate1P(sequence, (i * 16) + stepIndexRun));// must be calculated on clock edge only
					}
				}
				else {
					for (int i = 0; i < 2; i += stepConfig)
						slideFromCV[i] = cv[phrase[phraseIndexRun]][(i * 16) + stepIndexRun];
					if (moveIndexRunMode(&stepIndexRun, lengths[phrase[phraseIndexRun]], runModeSeq[phrase[phraseIndexRun]], &stepIndexRunHistory)) {
						moveIndexRunMode(&phraseIndexRun, phrases, runModeSong, &phraseIndexRunHistory);
						stepIndexRun = (runModeSeq[phrase[phraseIndexRun]] == MODE_REV ? lengths[phrase[phraseIndexRun]] - 1 : 0);// must always refresh after phraseIndexRun has changed
					}
					for (int i = 0; i < 2; i += stepConfig) {
						slideToCV[i] = cv[phrase[phraseIndexRun]][(i * 16) + stepIndexRun];
						gate1RandomEnable[i] = calcGate1RandomEnable(getGate1P(phrase[phraseIndexRun], (i * 16) + stepIndexRun));// must be calculated on clock edge only
					}
				}
				
				// Slide
				for (int i = 0; i < 2; i += stepConfig) {
					if ( ( editingSequence && ((attributes[sequence][(i * 16) + stepIndexRun] & ATT_MSK_SLIDE) != 0) ) || 
				         (!editingSequence && ((attributes[phrase[phraseIndexRun]][(i * 16) + stepIndexRun] & ATT_MSK_SLIDE) != 0) ) ) {
						// avtivate sliding (slideStepsRemain can be reset, else runs down to 0, either way, no need to reinit)
						slideStepsRemain[i] =   (unsigned long) (((float)clockPeriod)   * params[SLIDE_KNOB_PARAM].value / 2.0f);// 0-T slide, where T is clock period (can be too long when user does clock gating)
						//slideStepsRemain[i] = (unsigned long)  (engineGetSampleRate() * params[SLIDE_KNOB_PARAM].value );// 0-2s slide
						slideCVdelta[i] = (slideToCV[i] - slideFromCV[i])/(float)slideStepsRemain[i];
					}
				}
			}
			clockPeriod = 0ul;
		}	
		clockPeriod++;
		
		// Reset
		if (resetTrigger.process(inputs[RESET_INPUT].value + params[RESET_PARAM].value)) {
			//stepIndexEdit = 0;
			//sequence = 0;
			initRun(stepConfig, true);// must be after sequence reset
			resetLight = 1.0f;
			displayState = DISP_NORMAL;
			clockTrigger.reset();
		}
		else
			resetLight -= (resetLight / lightLambda) * engineGetSampleTime();
		
		
		//********** Outputs and lights **********
				
		// CV and gates outputs
		int seq = editingSequence ? (sequence) : (running ? phrase[phraseIndexRun] : phrase[phraseIndexEdit]);
		int step = editingSequence ? (running ? stepIndexRun : stepIndexEdit) : (stepIndexRun);
		if (running) {
			float slideOffset[2];
			for (int i = 0; i < 2; i += stepConfig)
				slideOffset[i] = (slideStepsRemain[i] > 0ul ? (slideCVdelta[i] * (float)slideStepsRemain[i]) : 0.0f);
			if (stepConfig == 1) {
				outputs[CVA_OUTPUT].value = cv[seq][step] - slideOffset[0];
				outputs[GATE1A_OUTPUT].value = (clockTrigger.isHigh() && gate1RandomEnable[0] && 
												((attributes[seq][step] & ATT_MSK_GATE1) != 0)) ? 10.0f : 0.0f;
				outputs[GATE2A_OUTPUT].value = (clockTrigger.isHigh() && 
												((attributes[seq][step] & ATT_MSK_GATE2) != 0)) ? 10.0f : 0.0f;
				outputs[CVB_OUTPUT].value = cv[seq][16 + step] - slideOffset[1];
				outputs[GATE1B_OUTPUT].value = (clockTrigger.isHigh() && gate1RandomEnable[1] && 
												((attributes[seq][16 + step] & ATT_MSK_GATE1) != 0)) ? 10.0f : 0.0f;
				outputs[GATE2B_OUTPUT].value = (clockTrigger.isHigh() && 
												((attributes[seq][16 + step] & ATT_MSK_GATE2) != 0)) ? 10.0f : 0.0f;
			} 
			else {
				outputs[CVA_OUTPUT].value = cv[seq][step] - slideOffset[0];
				outputs[GATE1A_OUTPUT].value = (clockTrigger.isHigh() && gate1RandomEnable[0] && 
												((attributes[seq][step] & ATT_MSK_GATE1) != 0)) ? 10.0f : 0.0f;
				outputs[GATE2A_OUTPUT].value = (clockTrigger.isHigh() && 
												((attributes[seq][step] & ATT_MSK_GATE2) != 0)) ? 10.0f : 0.0f;
				outputs[CVB_OUTPUT].value = 0.0f;
				outputs[GATE1B_OUTPUT].value = 0.0f;
				outputs[GATE2B_OUTPUT].value = 0.0f;
			}
		}
		else {// not running 
			if (editingChannel == 0) {
				outputs[CVA_OUTPUT].value = (editingGate > 0ul) ? editingGateCV : cv[seq][step];
				outputs[GATE1A_OUTPUT].value = (editingGate > 0ul && getGate1(seq, step)) ? 10.0f : 0.0f;
				outputs[GATE2A_OUTPUT].value = (editingGate > 0ul && getGate2(seq, step)) ? 10.0f : 0.0f;
				outputs[CVB_OUTPUT].value = 0.0f;
				outputs[GATE1B_OUTPUT].value = 0.0f;
				outputs[GATE2B_OUTPUT].value = 0.0f;
			}
			else {
				outputs[CVA_OUTPUT].value = 0.0f;
				outputs[GATE1A_OUTPUT].value = 0.0f;
				outputs[GATE2A_OUTPUT].value = 0.0f;
				outputs[CVB_OUTPUT].value = (editingGate > 0ul) ? editingGateCV : cv[seq][step];
				outputs[GATE1B_OUTPUT].value = (editingGate > 0ul && getGate1(seq, step)) ? 10.0f : 0.0f;
				outputs[GATE2B_OUTPUT].value = (editingGate > 0ul && getGate2(seq, step)) ? 10.0f : 0.0f;
			}
		}
		
		// Step/phrase lights
		if (infoCopyPaste != 0l) {
			for (int i = 0; i < 32; i++) {
				if ( (i >= stepIndexEdit && i < (stepIndexEdit + countCP)) || (countCP == 32) )
					lights[STEP_PHRASE_LIGHTS + (i<<1)].value = 0.5f;// Green when copy interval
				else
					lights[STEP_PHRASE_LIGHTS + (i<<1)].value = 0.0f; // Green (nothing)
				lights[STEP_PHRASE_LIGHTS + (i<<1) + 1].value = 0.0f;// Red (nothing)
			}
		}
		else {
			for (int i = 0; i < 32; i++) {
				if (displayState == DISP_LENGTH) {
					if (editingSequence) {
						int col = i % (16 * stepConfig);
						if (col < (lengths[sequence] - 1))
							setGreenRed(STEP_PHRASE_LIGHTS + i * 2, 0.1f, 0.0f);
						else if (col == (lengths[sequence] - 1))
							setGreenRed(STEP_PHRASE_LIGHTS + i * 2, 1.0f, 0.0f);
						else 
							setGreenRed(STEP_PHRASE_LIGHTS + i * 2, 0.0f, 0.0f);
					}
					else {
						if (i < phrases - 1)
							setGreenRed(STEP_PHRASE_LIGHTS + i * 2, 0.1f, 0.0f);
						else
							setGreenRed(STEP_PHRASE_LIGHTS + i * 2, (i == phrases - 1) ? 1.0f : 0.0f, 0.0f);
					}
				}
				else {// normal led display (i.e. not length)
					float red = 0.0f;
					float green = 0.0f;
					
					// Run cursor (green)
					if (editingSequence)
						green = ((running && (i % (16 * stepConfig)) == stepIndexRun)) ? 1.0f : 0.0f;
					else {
						green = ((running && (i == phraseIndexRun)) ? 1.0f : 0.0f);
						green += ((running && ((i % (16 * stepConfig)) == stepIndexRun) && i != phraseIndexEdit) ? 0.1f : 0.0f);
						green = clamp(green, 0.0f, 1.0f);
					}
					// Edit cursor (red)
					if (editingSequence)
						red = (i == stepIndexEdit ? 1.0f : 0.0f);
					else
						red = (i == phraseIndexEdit ? 1.0f : 0.0f);
					
					setGreenRed(STEP_PHRASE_LIGHTS + i * 2, green, red);
				}
			}
		}
	
		// Octave lights
		float octCV = 0.0f;
		if (editingSequence)
			octCV = cv[sequence][stepIndexEdit];
		else
			octCV = cv[phrase[phraseIndexEdit]][stepIndexRun];
		int octLightIndex = (int) floor(octCV + 3.0f);
		for (int i = 0; i < 7; i++) {
			if (!editingSequence && (!attached || !running || (stepConfig == 1)))// no oct lights when song mode and either (detached [1] or stopped [2] or 2x16config [3])
											// [1] makes no sense, can't mod steps and stepping though seq that may not be playing
											// [2] CV is set to 0V when not running and in song mode, so cv[][] makes no sense to display
											// [3] makes no sense, which sequence would be displayed, top or bottom row!
				lights[OCTAVE_LIGHTS + i].value = 0.0f;
			else {
				if (tiedWarning > 0l) {
					bool warningFlashState = calcTiedWarning(tiedWarning, tiedWarningInit);
					lights[OCTAVE_LIGHTS + i].value = (warningFlashState && (i == (6 - octLightIndex))) ? 1.0f : 0.0f;
				}
				else				
					lights[OCTAVE_LIGHTS + i].value = (i == (6 - octLightIndex) ? 1.0f : 0.0f);
			}
		}
		
		// Keyboard lights (can only show channel A when running attached in 1x16 mode, does not pose problem for all other situations)
		float cvValOffset;
		if (editingSequence) 
			cvValOffset = cv[sequence][stepIndexEdit] + 10.0f;//to properly handle negative note voltages
		else	
			cvValOffset = cv[phrase[phraseIndexEdit]][stepIndexRun] + 10.0f;//to properly handle negative note voltages
		int keyLightIndex = (int) clamp(  roundf( (cvValOffset-floor(cvValOffset)) * 12.0f ),  0.0f,  11.0f);
		for (int i = 0; i < 12; i++) {
			if (!editingSequence && (!attached || !running || (stepConfig == 1)))// no oct lights when song mode and either (detached [1] or stopped [2] or 2x16config [3])
											// [1] makes no sense, can't mod steps and stepping though seq that may not be playing
											// [2] CV is set to 0V when not running and in song mode, so cv[][] makes no sense to display
											// [3] makes no sense, which sequence would be displayed, top or bottom row!
				lights[KEY_LIGHTS + i].value = 0.0f;
			else {
				if (tiedWarning > 0l) {
					bool warningFlashState = calcTiedWarning(tiedWarning, tiedWarningInit);
					lights[KEY_LIGHTS + i].value = (warningFlashState && i == keyLightIndex) ? 1.0f : 0.0f;
				}
				else				
					lights[KEY_LIGHTS + i].value = (i == keyLightIndex ? 1.0f : 0.0f);
			}
		}			
		
		// Gate1, Gate1Prob, Gate2, Slide and Tied lights (can only show channel A when running attached in 1x16 mode, does not pose problem for all other situations)
		int attributesVal = attributes[sequence][stepIndexEdit];
		if (!editingSequence)
			attributesVal = attributes[phrase[phraseIndexEdit]][stepIndexRun];
		//
		lights[GATE1_LIGHT].value = ((attributesVal & ATT_MSK_GATE1) != 0) ? 1.0f : 0.0f;
		lights[GATE1_PROB_LIGHT].value = ((attributesVal & ATT_MSK_GATE1P) != 0) ? 1.0f : 0.0f;
		lights[GATE2_LIGHT].value = ((attributesVal & ATT_MSK_GATE2) != 0) ? 1.0f : 0.0f;
		lights[SLIDE_LIGHT].value = ((attributesVal & ATT_MSK_SLIDE) != 0) ? 1.0f : 0.0f;
		if (tiedWarning > 0l) {
			bool warningFlashState = calcTiedWarning(tiedWarning, tiedWarningInit);
			lights[TIE_LIGHT].value = (warningFlashState) ? 1.0f : 0.0f;
		}
		else
			lights[TIE_LIGHT].value = ((attributesVal & ATT_MSK_TIED) != 0) ? 1.0f : 0.0f;

		// Attach light
		lights[ATTACH_LIGHT].value = (running && attached) ? 1.0f : 0.0f;
		
		// Reset light
		lights[RESET_LIGHT].value =	resetLight;	
		
		// Run light
		lights[RUN_LIGHT].value = running;

		if (editingGate > 0ul)
			editingGate--;
		if (infoCopyPaste != 0l) {
			if (infoCopyPaste > 0l)
				infoCopyPaste --;
			if (infoCopyPaste < 0l)
				infoCopyPaste ++;
		}
		for (int i = 0; i < 2; i++)
			if (slideStepsRemain[i] > 0ul)
				slideStepsRemain[i]--;
		if (clockIgnoreOnReset > 0l)
			clockIgnoreOnReset--;
		if (tiedWarning > 0l)
			tiedWarning--;
		if (revertDisplay > 0l) {
			if (revertDisplay == 1)
				displayState = DISP_NORMAL;
			revertDisplay--;
		}

	}// step()
	
	void setGreenRed(int id, float green, float red) {
		lights[id + 0].value = green;
		lights[id + 1].value = red;
	}

	bool calcTiedWarning(long tiedWarning, long tiedWarningInit) {
		bool warningFlashState = true;
		if (tiedWarning > (tiedWarningInit * 2l / 4l) && tiedWarning < (tiedWarningInit * 3l / 4l))
			warningFlashState = false;
		else if (tiedWarning < (tiedWarningInit * 1l / 4l))
			warningFlashState = false;
		return warningFlashState;
	}	
	
	void applyTiedStep(int seqNum, int indexTied, int seqLength) {
		// Start on indexTied and loop until seqLength
		// Called because either:
		//   case A: tied was activated for given step
		//   case B: the given step's CV was modified
		// These cases are mutually exclusive
		
		// copy previous CV over to current step if tied
		if (getTied(seqNum,indexTied) && (indexTied > 0))
			cv[seqNum][indexTied] = cv[seqNum][indexTied - 1];
		
		// Affect downstream CVs of subsequent tied note chain (can be 0 length if next note is not tied)
		for (int i = indexTied + 1; i < seqLength && getTied(seqNum,i); i++) 
			cv[seqNum][i] = cv[seqNum][indexTied];
	}
};



struct PhraseSeq32Widget : ModuleWidget {
	PhraseSeq32 *module;
	DynamicSVGPanelEx *panel;
	int oldExpansion;
	IMPort* expPorts[5];
	
	struct SequenceDisplayWidget : TransparentWidget {
		PhraseSeq32 *module;
		std::shared_ptr<Font> font;
		char displayStr[4];
		std::string modeLabels[5]={"FWD","REV","PPG","BRN","RND"};
		
		SequenceDisplayWidget() {
			font = Font::load(assetPlugin(plugin, "res/fonts/Segment14.ttf"));
		}
		
		void runModeToStr(int num) {
			if (num >= 0 && num < 5)
				snprintf(displayStr, 4, "%s", modeLabels[num].c_str());
		}

		void draw(NVGcontext *vg) override {
			NVGcolor textColor = prepareDisplay(vg, &box);
			nvgFontFaceId(vg, font->handle);
			//nvgTextLetterSpacing(vg, 2.5);

			Vec textPos = Vec(6, 24);
			nvgFillColor(vg, nvgTransRGBA(textColor, 16));
			nvgText(vg, textPos.x, textPos.y, "~~~", NULL);
			nvgFillColor(vg, textColor);
			if (module->infoCopyPaste != 0l) {
				if (module->infoCopyPaste > 0l) {// if copy display "CPY"
					snprintf(displayStr, 4, "CPY");
				}
				else {// if paste display "PST"
					snprintf(displayStr, 4, "PST");
				}
			}
			else {
				if (module->displayState == PhraseSeq32::DISP_MODE) {
					if (module->editingSequence)
						runModeToStr(module->runModeSeq[module->sequence]);
					else
						runModeToStr(module->runModeSong);
				}
				else if (module->displayState == PhraseSeq32::DISP_LENGTH) {
					if (module->editingSequence)
						snprintf(displayStr, 4, "L%2u", (unsigned) module->lengths[module->sequence]);
					else
						snprintf(displayStr, 4, "L%2u", (unsigned) module->phrases);
				}
				else if (module->displayState == PhraseSeq32::DISP_TRANSPOSE) {
					snprintf(displayStr, 4, "+%2u", (unsigned) abs(module->transposeOffset));
					if (module->transposeOffset < 0)
						displayStr[0] = '-';
				}
				else if (module->displayState == PhraseSeq32::DISP_ROTATE) {
					snprintf(displayStr, 4, ")%2u", (unsigned) abs(module->rotateOffset));
					if (module->rotateOffset < 0)
						displayStr[0] = '(';
				}
				else {// DISP_NORMAL
					snprintf(displayStr, 4, " %2u", (unsigned) (module->editingSequence ? 
						module->sequence : module->phrase[module->phraseIndexEdit]) + 1 );
				}
			}
			displayStr[3] = 0;// more safety
			nvgText(vg, textPos.x, textPos.y, displayStr, NULL);
		}
	};		
	
	struct PanelThemeItem : MenuItem {
		PhraseSeq32 *module;
		int theme;
		void onAction(EventAction &e) override {
			module->panelTheme = theme;
		}
		void step() override {
			rightText = (module->panelTheme == theme) ? "✔" : "";
		}
	};
	struct ExpansionItem : MenuItem {
		PhraseSeq32 *module;
		void onAction(EventAction &e) override {
			module->expansion = module->expansion == 1 ? 0 : 1;
		}
	};
	struct ResetOnRunItem : MenuItem {
		PhraseSeq32 *module;
		void onAction(EventAction &e) override {
			module->resetOnRun = !module->resetOnRun;
		}
	};
	Menu *createContextMenu() override {
		Menu *menu = ModuleWidget::createContextMenu();

		MenuLabel *spacerLabel = new MenuLabel();
		menu->addChild(spacerLabel);

		PhraseSeq32 *module = dynamic_cast<PhraseSeq32*>(this->module);
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
		
		MenuLabel *expansionLabel = new MenuLabel();
		expansionLabel->text = "Expansion module";
		menu->addChild(expansionLabel);

		ExpansionItem *expItem = MenuItem::create<ExpansionItem>(expansionMenuLabel, CHECKMARK(module->expansion != 0));
		expItem->module = module;
		menu->addChild(expItem);
		
		menu->addChild(new MenuLabel());// empty line
		
		MenuLabel *settingsLabel = new MenuLabel();
		settingsLabel->text = "Settings";
		menu->addChild(settingsLabel);
		
		ResetOnRunItem *rorItem = MenuItem::create<ResetOnRunItem>("Reset on Run", CHECKMARK(module->resetOnRun));
		rorItem->module = module;
		menu->addChild(rorItem);
		
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
	
	PhraseSeq32Widget(PhraseSeq32 *module) : ModuleWidget(module) {
		this->module = module;
		oldExpansion = -1;
		
		// Main panel from Inkscape
        panel = new DynamicSVGPanelEx();
        panel->mode = &module->panelTheme;
        panel->expansion = &module->expansion;
        panel->addPanel(SVG::load(assetPlugin(plugin, "res/light/PhraseSeq32.svg")));
        panel->addPanel(SVG::load(assetPlugin(plugin, "res/dark/PhraseSeq32_dark.svg")));
        panel->addPanelEx(SVG::load(assetPlugin(plugin, "res/light/PhraseSeqExp.svg")));
        panel->addPanelEx(SVG::load(assetPlugin(plugin, "res/dark/PhraseSeqExp_dark.svg")));
        box.size = panel->box.size;
        addChild(panel);
		
		// Screws
		addChild(createDynamicScrew<IMScrew>(Vec(15, 0), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(15, 365), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x-30, 0), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x-30, 365), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x+30, 0), &module->panelTheme));
		addChild(createDynamicScrew<IMScrew>(Vec(box.size.x+30, 365), &module->panelTheme));

		
		
		// ****** Top row ******
		
		static const int rowRulerT0 = 48;
		static const int columnRulerT0 = 18;//30;// Step/Phase LED buttons
		static const int columnRulerT3 = 377;// Attach 
		static const int columnRulerT4 = 430;// Config 

		// Step/Phrase LED buttons
		int posX = columnRulerT0;
		static int spacingSteps = 20;
		static int spacingSteps4 = 4;
		for (int x = 0; x < 16; x++) {
			// First row
			addParam(ParamWidget::create<LEDButton>(Vec(posX, rowRulerT0 - 10 + 3 - 4.4f), module, PhraseSeq32::STEP_PHRASE_PARAMS + x, 0.0f, 1.0f, 0.0f));
			addChild(ModuleLightWidget::create<MediumLight<GreenRedLight>>(Vec(posX + 4.4f, rowRulerT0 - 10 + 3), module, PhraseSeq32::STEP_PHRASE_LIGHTS + (x * 2)));
			// Second row
			addParam(ParamWidget::create<LEDButton>(Vec(posX, rowRulerT0 + 10 + 3 - 4.4f), module, PhraseSeq32::STEP_PHRASE_PARAMS + x + 16, 0.0f, 1.0f, 0.0f));
			addChild(ModuleLightWidget::create<MediumLight<GreenRedLight>>(Vec(posX + 4.4f, rowRulerT0 + 10 + 3), module, PhraseSeq32::STEP_PHRASE_LIGHTS + ((x + 16) * 2)));
			// step position to next location and handle groups of four
			posX += spacingSteps;
			if ((x + 1) % 4 == 0)
				posX += spacingSteps4;
		}
		// Attach button and light
		addParam(ParamWidget::create<TL1105>(Vec(columnRulerT3 - 4, rowRulerT0 - 6 + 2 + offsetTL1105), module, PhraseSeq32::ATTACH_PARAM, 0.0f, 1.0f, 0.0f));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(columnRulerT3 + 12 + offsetMediumLight, rowRulerT0 - 6 + offsetMediumLight), module, PhraseSeq32::ATTACH_LIGHT));		
		// Config switch
		addParam(ParamWidget::create<CKSS>(Vec(columnRulerT4 + hOffsetCKSS + 1, rowRulerT0 - 6 + vOffsetCKSS), module, PhraseSeq32::CONFIG_PARAM, 0.0f, 1.0f, PhraseSeq32::CONFIG_PARAM_INIT_VALUE));

		
		
		// ****** Octave and keyboard area ******
		
		// Octave LED buttons
		static const float octLightsIntY = 20.0f;
		for (int i = 0; i < 7; i++) {
			addParam(ParamWidget::create<LEDButton>(Vec(15 + 3, 82 + 24 + i * octLightsIntY- 4.4f), module, PhraseSeq32::OCTAVE_PARAM + i, 0.0f, 1.0f, 0.0f));
			addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(15 + 3 + 4.4f, 82 + 24 + i * octLightsIntY), module, PhraseSeq32::OCTAVE_LIGHTS + i));
		}
		// Keys and Key lights
		static const int keyNudgeX = 7;
		static const int keyNudgeY = 2;
		static const int offsetKeyLEDx = 6;
		static const int offsetKeyLEDy = 28;
		// Black keys and lights
		addParam(ParamWidget::create<InvisibleKeySmall>(			Vec(65+keyNudgeX, 89+keyNudgeY), module, PhraseSeq32::KEY_PARAMS + 1, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(65+keyNudgeX+offsetKeyLEDx, 89+keyNudgeY+offsetKeyLEDy), module, PhraseSeq32::KEY_LIGHTS + 1));
		addParam(ParamWidget::create<InvisibleKeySmall>(			Vec(93+keyNudgeX, 89+keyNudgeY), module, PhraseSeq32::KEY_PARAMS + 3, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(93+keyNudgeX+offsetKeyLEDx, 89+keyNudgeY+offsetKeyLEDy), module, PhraseSeq32::KEY_LIGHTS + 3));
		addParam(ParamWidget::create<InvisibleKeySmall>(			Vec(150+keyNudgeX, 89+keyNudgeY), module, PhraseSeq32::KEY_PARAMS + 6, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(150+keyNudgeX+offsetKeyLEDx, 89+keyNudgeY+offsetKeyLEDy), module, PhraseSeq32::KEY_LIGHTS + 6));
		addParam(ParamWidget::create<InvisibleKeySmall>(			Vec(178+keyNudgeX, 89+keyNudgeY), module, PhraseSeq32::KEY_PARAMS + 8, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(178+keyNudgeX+offsetKeyLEDx, 89+keyNudgeY+offsetKeyLEDy), module, PhraseSeq32::KEY_LIGHTS + 8));
		addParam(ParamWidget::create<InvisibleKeySmall>(			Vec(206+keyNudgeX, 89+keyNudgeY), module, PhraseSeq32::KEY_PARAMS + 10, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(206+keyNudgeX+offsetKeyLEDx, 89+keyNudgeY+offsetKeyLEDy), module, PhraseSeq32::KEY_LIGHTS + 10));
		// White keys and lights
		addParam(ParamWidget::create<InvisibleKeySmall>(			Vec(51+keyNudgeX, 139+keyNudgeY), module, PhraseSeq32::KEY_PARAMS + 0, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(51+keyNudgeX+offsetKeyLEDx, 139+keyNudgeY+offsetKeyLEDy), module, PhraseSeq32::KEY_LIGHTS + 0));
		addParam(ParamWidget::create<InvisibleKeySmall>(			Vec(79+keyNudgeX, 139+keyNudgeY), module, PhraseSeq32::KEY_PARAMS + 2, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(79+keyNudgeX+offsetKeyLEDx, 139+keyNudgeY+offsetKeyLEDy), module, PhraseSeq32::KEY_LIGHTS + 2));
		addParam(ParamWidget::create<InvisibleKeySmall>(			Vec(107+keyNudgeX, 139+keyNudgeY), module, PhraseSeq32::KEY_PARAMS + 4, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(107+keyNudgeX+offsetKeyLEDx, 139+keyNudgeY+offsetKeyLEDy), module, PhraseSeq32::KEY_LIGHTS + 4));
		addParam(ParamWidget::create<InvisibleKeySmall>(			Vec(136+keyNudgeX, 139+keyNudgeY), module, PhraseSeq32::KEY_PARAMS + 5, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(136+keyNudgeX+offsetKeyLEDx, 139+keyNudgeY+offsetKeyLEDy), module, PhraseSeq32::KEY_LIGHTS + 5));
		addParam(ParamWidget::create<InvisibleKeySmall>(			Vec(164+keyNudgeX, 139+keyNudgeY), module, PhraseSeq32::KEY_PARAMS + 7, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(164+keyNudgeX+offsetKeyLEDx, 139+keyNudgeY+offsetKeyLEDy), module, PhraseSeq32::KEY_LIGHTS + 7));
		addParam(ParamWidget::create<InvisibleKeySmall>(			Vec(192+keyNudgeX, 139+keyNudgeY), module, PhraseSeq32::KEY_PARAMS + 9, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(192+keyNudgeX+offsetKeyLEDx, 139+keyNudgeY+offsetKeyLEDy), module, PhraseSeq32::KEY_LIGHTS + 9));
		addParam(ParamWidget::create<InvisibleKeySmall>(			Vec(220+keyNudgeX, 139+keyNudgeY), module, PhraseSeq32::KEY_PARAMS + 11, 0.0, 1.0, 0.0));
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(220+keyNudgeX+offsetKeyLEDx, 139+keyNudgeY+offsetKeyLEDy), module, PhraseSeq32::KEY_LIGHTS + 11));
		
		
		
		// ****** Right side control area ******

		static const int rowRulerMK0 = 101;// Edit mode row
		static const int rowRulerMK1 = rowRulerMK0 + 56; // Run row
		static const int rowRulerMK2 = rowRulerMK1 + 54; // Copy-paste Tran/rot row
		static const int columnRulerMK0 = 278;// Edit mode column
		static const int columnRulerMK2 = columnRulerT4;// Mode/Len column
		static const int columnRulerMK1 = 353;// Display column
		
		// Edit mode switch
		addParam(ParamWidget::create<CKSS>(Vec(columnRulerMK0 + hOffsetCKSS, rowRulerMK0 + vOffsetCKSS), module, PhraseSeq32::EDIT_PARAM, 0.0f, 1.0f, PhraseSeq32::EDIT_PARAM_INIT_VALUE));
		// Sequence display
		SequenceDisplayWidget *displaySequence = new SequenceDisplayWidget();
		displaySequence->box.pos = Vec(columnRulerMK1-15, rowRulerMK0 + 3 + vOffsetDisplay);
		displaySequence->box.size = Vec(55, 30);// 3 characters
		displaySequence->module = module;
		addChild(displaySequence);
		// Run mode button
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerMK2 + offsetCKD6b, rowRulerMK0 + 0 + offsetCKD6b), module, PhraseSeq32::RUNMODE_PARAM, 0.0f, 1.0f, 0.0f, &module->panelTheme));

		// Autostep
		addParam(ParamWidget::create<CKSS>(Vec(columnRulerMK0 + hOffsetCKSS, rowRulerMK1 + 7 + vOffsetCKSS), module, PhraseSeq32::AUTOSTEP_PARAM, 0.0f, 1.0f, 1.0f));		
		// Sequence knob
		addParam(createDynamicParam<IMBigKnobInf>(Vec(columnRulerMK1 + 1 + offsetIMBigKnob, rowRulerMK0 + 55 + offsetIMBigKnob), module, PhraseSeq32::SEQUENCE_PARAM, -INFINITY, INFINITY, 0.0f, &module->panelTheme));		
		// Transpose/rotate button
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerMK2 + offsetCKD6b, rowRulerMK1 + 4 + offsetCKD6b), module, PhraseSeq32::TRAN_ROT_PARAM, 0.0f, 1.0f, 0.0f, &module->panelTheme));
		
		// Reset LED bezel and light
		addParam(ParamWidget::create<LEDBezel>(Vec(columnRulerMK0 - 15 + offsetLEDbezel, rowRulerMK2 + 5 + offsetLEDbezel), module, PhraseSeq32::RESET_PARAM, 0.0f, 1.0f, 0.0f));
		addChild(ModuleLightWidget::create<MuteLight<GreenLight>>(Vec(columnRulerMK0 - 15 + offsetLEDbezel + offsetLEDbezelLight, rowRulerMK2 + 5 + offsetLEDbezel + offsetLEDbezelLight), module, PhraseSeq32::RESET_LIGHT));
		// Run LED bezel and light
		addParam(ParamWidget::create<LEDBezel>(Vec(columnRulerMK0 + 20 + offsetLEDbezel, rowRulerMK2 + 5 + offsetLEDbezel), module, PhraseSeq32::RUN_PARAM, 0.0f, 1.0f, 0.0f));
		addChild(ModuleLightWidget::create<MuteLight<GreenLight>>(Vec(columnRulerMK0 + 20 + offsetLEDbezel + offsetLEDbezelLight, rowRulerMK2 + 5 + offsetLEDbezel + offsetLEDbezelLight), module, PhraseSeq32::RUN_LIGHT));
		// Copy/paste buttons
		addParam(ParamWidget::create<TL1105>(Vec(columnRulerMK1 - 10, rowRulerMK2 + 5 + offsetTL1105), module, PhraseSeq32::COPY_PARAM, 0.0f, 1.0f, 0.0f));
		addParam(ParamWidget::create<TL1105>(Vec(columnRulerMK1 + 20, rowRulerMK2 + 5 + offsetTL1105), module, PhraseSeq32::PASTE_PARAM, 0.0f, 1.0f, 0.0f));
		// Copy-paste mode switch (3 position)
		addParam(ParamWidget::create<CKSSThreeInv>(Vec(columnRulerMK2 + hOffsetCKSS + 1, rowRulerMK2 - 3 + vOffsetCKSSThree), module, PhraseSeq32::CPMODE_PARAM, 0.0f, 2.0f, 2.0f));	// 0.0f is top position

		
		
		// ****** Gate and slide section ******
		
		static const int rowRulerMB0 = 214;
		static const int columnRulerMBspacing = 70;
		static const int columnRulerMB2 = 130;// Gate2
		static const int columnRulerMB1 = columnRulerMB2 - columnRulerMBspacing;// Gate1 
		static const int columnRulerMB3 = columnRulerMB2 + columnRulerMBspacing;// Tie
		static const int posLEDvsButton = + 25;
		
		// Gate 1 light and button
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(columnRulerMB1 + posLEDvsButton + offsetMediumLight, rowRulerMB0 + 4 + offsetMediumLight), module, PhraseSeq32::GATE1_LIGHT));		
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerMB1 + offsetCKD6b, rowRulerMB0 + 4 + offsetCKD6b), module, PhraseSeq32::GATE1_PARAM, 0.0f, 1.0f, 0.0f, &module->panelTheme));
		// Gate 2 light and button
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(columnRulerMB2 + posLEDvsButton + offsetMediumLight, rowRulerMB0 + 4 + offsetMediumLight), module, PhraseSeq32::GATE2_LIGHT));		
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerMB2 + offsetCKD6b, rowRulerMB0 + 4 + offsetCKD6b), module, PhraseSeq32::GATE2_PARAM, 0.0f, 1.0f, 0.0f, &module->panelTheme));
		// Tie light and button
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(columnRulerMB3 + posLEDvsButton + offsetMediumLight, rowRulerMB0 + 4 + offsetMediumLight), module, PhraseSeq32::TIE_LIGHT));		
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerMB3 + offsetCKD6b, rowRulerMB0 + 4 + offsetCKD6b), module, PhraseSeq32::TIE_PARAM, 0.0f, 1.0f, 0.0f, &module->panelTheme));

						
		
		// ****** Bottom two rows ******
		
		static const int inputJackSpacingX = 54;
		static const int outputJackSpacingX = 45;
		static const int rowRulerB0 = 323;
		static const int rowRulerB1 = 269;
		static const int columnRulerB0 = 22;
		static const int columnRulerB1 = columnRulerB0 + inputJackSpacingX;
		static const int columnRulerB2 = columnRulerB1 + inputJackSpacingX;
		static const int columnRulerB3 = columnRulerB2 + inputJackSpacingX;
		static const int columnRulerB4 = columnRulerB3 + inputJackSpacingX;
		static const int columnRulerB8 = columnRulerMK2 + 1;
		static const int columnRulerB7 = columnRulerB8 - outputJackSpacingX;
		static const int columnRulerB6 = columnRulerB7 - outputJackSpacingX;
		static const int columnRulerB5 = columnRulerB6 - outputJackSpacingX - 4;// clock and reset

		
		// Gate 1 probability light and button
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(columnRulerB0 + posLEDvsButton + offsetMediumLight, rowRulerB1 + offsetMediumLight), module, PhraseSeq32::GATE1_PROB_LIGHT));		
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerB0 + offsetCKD6b, rowRulerB1 + offsetCKD6b), module, PhraseSeq32::GATE1_PROB_PARAM, 0.0f, 1.0f, 0.0f, &module->panelTheme));
		// Gate 1 probability knob
		addParam(createDynamicParam<IMSmallKnob>(Vec(columnRulerB1 + offsetIMSmallKnob, rowRulerB1 + offsetIMSmallKnob), module, PhraseSeq32::GATE1_KNOB_PARAM, 0.0f, 1.0f, 1.0f, &module->panelTheme));
		// Slide light and button
		addChild(ModuleLightWidget::create<MediumLight<RedLight>>(Vec(columnRulerB2 + posLEDvsButton + offsetMediumLight, rowRulerB1 + offsetMediumLight), module, PhraseSeq32::SLIDE_LIGHT));		
		addParam(createDynamicParam<IMBigPushButton>(Vec(columnRulerB2 + offsetCKD6b, rowRulerB1 + offsetCKD6b), module, PhraseSeq32::SLIDE_BTN_PARAM, 0.0f, 1.0f, 0.0f, &module->panelTheme));
		// Slide knob
		addParam(createDynamicParam<IMSmallKnob>(Vec(columnRulerB3 + offsetIMSmallKnob, rowRulerB1 + offsetIMSmallKnob), module, PhraseSeq32::SLIDE_KNOB_PARAM, 0.0f, 2.0f, 0.2f, &module->panelTheme));
		// CV in
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB4, rowRulerB1), Port::INPUT, module, PhraseSeq32::CV_INPUT, &module->panelTheme));
		// Clock input
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB5, rowRulerB1), Port::INPUT, module, PhraseSeq32::CLOCK_INPUT, &module->panelTheme));
		// Channel A outputs
		addOutput(createDynamicPort<IMPort>(Vec(columnRulerB6, rowRulerB1), Port::OUTPUT, module, PhraseSeq32::CVA_OUTPUT, &module->panelTheme));
		addOutput(createDynamicPort<IMPort>(Vec(columnRulerB7, rowRulerB1), Port::OUTPUT, module, PhraseSeq32::GATE1A_OUTPUT, &module->panelTheme));
		addOutput(createDynamicPort<IMPort>(Vec(columnRulerB8, rowRulerB1), Port::OUTPUT, module, PhraseSeq32::GATE2A_OUTPUT, &module->panelTheme));


		// CV control Inputs 
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB0, rowRulerB0), Port::INPUT, module, PhraseSeq32::LEFTCV_INPUT, &module->panelTheme));
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB1, rowRulerB0), Port::INPUT, module, PhraseSeq32::RIGHTCV_INPUT, &module->panelTheme));
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB2, rowRulerB0), Port::INPUT, module, PhraseSeq32::SEQCV_INPUT, &module->panelTheme));
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB3, rowRulerB0), Port::INPUT, module, PhraseSeq32::RUNCV_INPUT, &module->panelTheme));
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB4, rowRulerB0), Port::INPUT, module, PhraseSeq32::WRITE_INPUT, &module->panelTheme));
		// Reset input
		addInput(createDynamicPort<IMPort>(Vec(columnRulerB5, rowRulerB0), Port::INPUT, module, PhraseSeq32::RESET_INPUT, &module->panelTheme));
		// Channel B outputs
		addOutput(createDynamicPort<IMPort>(Vec(columnRulerB6, rowRulerB0), Port::OUTPUT, module, PhraseSeq32::CVB_OUTPUT, &module->panelTheme));
		addOutput(createDynamicPort<IMPort>(Vec(columnRulerB7, rowRulerB0), Port::OUTPUT, module, PhraseSeq32::GATE1B_OUTPUT, &module->panelTheme));
		addOutput(createDynamicPort<IMPort>(Vec(columnRulerB8, rowRulerB0), Port::OUTPUT, module, PhraseSeq32::GATE2B_OUTPUT, &module->panelTheme));
		
		
		// Expansion module
		static const int rowRulerExpTop = 65;
		static const int rowSpacingExp = 60;
		static const int colRulerExp = 497;
		addInput(expPorts[0] = createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 0), Port::INPUT, module, PhraseSeq32::GATE1CV_INPUT, &module->panelTheme));
		addInput(expPorts[1] = createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 1), Port::INPUT, module, PhraseSeq32::GATE2CV_INPUT, &module->panelTheme));
		addInput(expPorts[2] = createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 2), Port::INPUT, module, PhraseSeq32::TIEDCV_INPUT, &module->panelTheme));
		addInput(expPorts[3] = createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 3), Port::INPUT, module, PhraseSeq32::SLIDECV_INPUT, &module->panelTheme));
		addInput(expPorts[4] = createDynamicPort<IMPort>(Vec(colRulerExp, rowRulerExpTop + rowSpacingExp * 4), Port::INPUT, module, PhraseSeq32::MODECV_INPUT, &module->panelTheme));
	}
};

Model *modelPhraseSeq32 = Model::create<PhraseSeq32, PhraseSeq32Widget>("Impromptu Modular", "Phrase-Seq-32", "SEQ - Phrase-Seq-32", SEQUENCER_TAG);

/*CHANGE LOG

0.6.7:
allow full edit capabilities in Seq and song mode
no reset on run by default, with switch added in context menu
reset does not revert seq or song number to 1
gate 2 is off by default
fix emitted monitoring gates to depend on gate states instead of always triggering

0.6.6:
config and knob bug fixes when loading patch

0.6.5:
paste 4, 8 doesn't loop over to overwrite first steps
small adjustements to gates and CVs used in monitoring write operations
add GATE1, GATE2, TIED, SLIDE CV inputs 
add MODE CV input (affects only selected sequence and in Seq mode)
add expansion panel option
swap MODE/LEN so that length happens first (update manual)

0.6.4:
initial release of PS32
*/
