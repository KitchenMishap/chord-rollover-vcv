#include "plugin.hpp"

const char* whiteNoteIntervals = "TTSTTTS";

std::string generateModeString(int mode)
{
	assert( mode >= 0 );
	assert( mode < 7 );

	std::string result = "0123456\0";
	for( int i=0; i<7; i++ ) {
		result[i] = whiteNoteIntervals[(i+ mode) % 7];
	}
	result[7] = '\0';
	return result;
}
int voltageToNearestSemi(float voltage)
{
	return (int)(round(voltage * 12.f));
}
int semiToSemiBaseOctave(int semi)
{
	int mod = semi % 12;
	if( mod < 0 ) {
		mod += 12;
	}
	return mod;
}
int semiToOctaveShift(int semi)
{
	return 12 * (semi / 12);
}
int semiToNoteWithinKeySig(int semi, int key, std::string modeString, bool *outOfKey)
{
	assert(semi >= 0);
	assert(semi <= 11);
	assert(key >= 0);
	assert(key <= 11);
	assert(modeString[7] == '\0');

	int semiToTry = key;
	*outOfKey = false;
 	for( int noteToTry = 0; noteToTry <= 6; noteToTry++ ) {
		if( semi == semiToTry ) {
			return noteToTry;
		}
		if( modeString[noteToTry] == 'T' ) {
			semiToTry += 2;
		} else {
			semiToTry += 1;
		}
		semiToTry = semiToTry % 12;
	}
	// No exact match. Find the match that works when we start a semi lower...
	*outOfKey = true;
	semiToTry = key;
	if( semi == 0 ) {
		semi = 11;
	} else {
		semi -= 1;
	}
	for( int noteToTry = 0; noteToTry <= 6; noteToTry++ ) {
		if( semi == semiToTry ) {
			return noteToTry;
		}
		if( modeString[noteToTry] == 'T' ) {
			semiToTry += 2;
		} else {
			semiToTry += 1;
		}
		semiToTry = semiToTry % 12;
	}
	// That didn't match either. Impossible!
	assert(false);
	return 0;
}

int noteToSemiWithinKeySig(int note, int key, std::string modeString)
{
	int semi = key;
	for( int stepNote = 1; stepNote <= note; stepNote++ ) {
		int modeIndex = (stepNote -1) % 7 ;
		if( modeString[modeIndex] == 'T' ) {
			semi += 2;
		} else {
			semi += 1;
		}
	}
	return semi;
}

void testKeyMode(int key, int mode)
{
	std::string modeString = generateModeString(mode);
	for( int semi = 0; semi <= 11; semi++ ) {
		bool outOfKey = false;
		int note = semiToNoteWithinKeySig(semi, key, modeString, &outOfKey);
		assert(note >= 0);
		assert(note <= 6);
	}
}

void runTests()
{
	assert(voltageToNearestSemi(0.f) == 0);
	assert(voltageToNearestSemi(1.f) == 12);
	assert(voltageToNearestSemi(-1.f) == -12);
	assert(voltageToNearestSemi(1.f / 12.f) == 1);
	assert(voltageToNearestSemi(11.f / 12.f) == 11);
	assert(semiToSemiBaseOctave(0) == 0);
	assert(semiToSemiBaseOctave(11) == 11);
	assert(semiToSemiBaseOctave(12) == 0);
	assert(semiToSemiBaseOctave(-11) == 1);
	assert(semiToSemiBaseOctave(-12) == 0);
	assert(semiToOctaveShift(0) == 0);
	assert(semiToOctaveShift(12) == 12);
	assert(semiToOctaveShift(-12) == -12);

	for( int key = 0; key < 12; key++ )
	{
		for( int mode = 0; mode < 7; mode++ )
		{
			testKeyMode(key,mode);
		}
	}

	bool outOfKey = false;
	std::string modeString = generateModeString(0);
	assert(semiToNoteWithinKeySig(0,0,modeString,&outOfKey)==0);	// C in C Major Ionian
	assert(outOfKey==false);
	assert(semiToNoteWithinKeySig(1,0,modeString,&outOfKey)==0);	// C# in C Major Ionian
	assert(outOfKey==true);
	assert(semiToNoteWithinKeySig(2,0,modeString,&outOfKey)==1);	// D in C Major Ionian
	assert(outOfKey==false);
	assert(semiToNoteWithinKeySig(9,11,modeString,&outOfKey)==5);	// A in B Major Ionian
	assert(outOfKey==true);
	modeString = generateModeString(1);
	assert(semiToNoteWithinKeySig(9,11,modeString,&outOfKey)==6);	// A in B Dorian
	assert(outOfKey==false);
}

int chordNotes[8] = {0,2,4,7,9,11,14,16};	// Room for a heptad plus first inversion

struct ChordRollover : Module {
	enum ParamId {
		KEYSIG_PARAM,
		CHORD_PARAM,
		TIME_PARAM,
		PROFILE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		VOCT_INPUT,
		GATE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		VOCT_OUTPUT,
		GATE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		ROLLOVER_LIGHT,
		LIGHTS_LEN
	};

	enum Profiles {
		STEP_PROFILE = 0,
		TRIANGLE_PROFILE,
		SINE_PROFILE,
		HALFSINE_PROFILE
	};

	dsp::SchmittTrigger trigger;
	dsp::PulseGenerator pulse;
	float prevPitch = 0.0;

	ChordRollover() {
		INFO("Running Tests...");
		runTests();
		INFO("...Tests completed");

		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(KEYSIG_PARAM, 0.f, 11.f, 0.f, "Key Signature", {"C", "C♯/D♭", "D", "D♯/E♭", "E", "F", "F♯/G♭", "G", "G♯/A♭", "A", "A♯/B♭", "B"});
		configSwitch(CHORD_PARAM, 1.f, 7.f, 4.f, "Notes in chord", {"Monad", "Diad", "Triad", "Tetrad", "Pentad", "Hexad", "Heptad"});
		configParam(TIME_PARAM, 0.f, 5.f, 0.5f, "Glide time (s)" );
		configSwitch(PROFILE_PARAM, (float)STEP_PROFILE, (float)HALFSINE_PROFILE, (float)TRIANGLE_PROFILE, "Glide profile", {"Step (Bypass)", "Triangle", "Sine", "Half Sine"});
		configInput(VOCT_INPUT, "(Mono) Pitch");
		configInput(GATE_INPUT, "(Mono) Gate");
		configOutput(VOCT_OUTPUT, "(Poly) Pitch");
		configOutput(GATE_OUTPUT, "(Poly) Gate");
	}

	void playChord() {
		int key = (int)(params[KEYSIG_PARAM].getValue());
		int mode = 0;	// Not implemented yet
		std::string modeString = generateModeString(mode);

		float voct = inputs[VOCT_INPUT].getVoltage();
		int semi = voltageToNearestSemi(voct);

		semi = semi % 12;
		if( semi < 0 ) {
			semi += 12;
		}

		bool outOfKey = false;
		int note = semiToNoteWithinKeySig(semi,key,modeString,&outOfKey);
		int firstInversion = outOfKey ? 1 : 0;

		int numNotes = (int)(params[CHORD_PARAM].getValue());

		std::vector<int> notes;
		for( int noteIndex = 0; noteIndex < numNotes; noteIndex++ ) {
			int thisNote = note + chordNotes[noteIndex + firstInversion];
			notes.push_back(thisNote);
		}

		std::vector<int> semis;
		for( int noteIndex = 0; noteIndex < numNotes; noteIndex++ ) {
			int thisSemi = noteToSemiWithinKeySig(notes[noteIndex], key, modeString);
			semis.push_back(thisSemi);
		}

		outputs[VOCT_OUTPUT].setChannels(numNotes);
		for( int noteIndex = 0; noteIndex < numNotes; noteIndex++ ) {
			outputs[VOCT_OUTPUT].setVoltage((float)(semis[noteIndex]) / 12.f , noteIndex);
		}
	}

	void process(const ProcessArgs& args) override {
		// Has the gate been triggered this sample?
		float gateV = inputs[GATE_INPUT].getVoltage();
		auto tc = trigger.processEvent(gateV, 0.1, 1.0);
		bool gateOn = (tc==dsp::SchmittTrigger::TRIGGERED);

		// Has the pitch changed more than 0.5 semitones this sample?
		float pitchV = inputs[VOCT_INPUT].getVoltage();
		bool pitchChange = abs(pitchV - prevPitch) > 1.f / 12.f / 2.f;
		prevPitch = pitchV;

		// Has the pitch changed without a gate trigger?
		bool rollover = pitchChange && !gateOn;

		// If so, flash the light
		if( rollover ) {
			pulse.trigger(0.1f);
		}
		bool light = pulse.process(args.sampleTime);
		lights[ROLLOVER_LIGHT].setBrightness(light ? 1.f : 0.f);

		float gate = inputs[GATE_INPUT].getVoltage();
		int numNotes = (int)(params[CHORD_PARAM].getValue());
		outputs[GATE_OUTPUT].setChannels(numNotes);
		for( int i=0; i<numNotes; i++ ) {
			outputs[GATE_OUTPUT].setVoltage(gate, i);
		}

		if( gateOn || pitchChange ) {
			playChord();
		}
	}
};

struct ChordRolloverWidget : ModuleWidget {
	ChordRolloverWidget(ChordRollover* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Panel.svg")));

		float width = 5.08 * 5;

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(width/2.f, 24)), module, ChordRollover::KEYSIG_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(width/2.f, 41)), module, ChordRollover::CHORD_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(width/2.f, 58)), module, ChordRollover::TIME_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(width/2.f, 75)), module, ChordRollover::PROFILE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7, 97)), module, ChordRollover::VOCT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(width - 7, 97)), module, ChordRollover::GATE_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7, 113)), module, ChordRollover::VOCT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(width - 7, 113)), module, ChordRollover::GATE_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(width - 7, 87)), module, ChordRollover::ROLLOVER_LIGHT));

		// mm2px(Vec(10.0, 10.0))
		addChild(createWidget<Widget>(mm2px(Vec(2.684, 81.945))));
	}
};

Model* modelChordRollover = createModel<ChordRollover, ChordRolloverWidget>("ChordRollover");