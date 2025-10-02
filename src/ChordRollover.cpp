#include "plugin.hpp"

const char* whiteNoteIntervals = "TTSTTTS";

int niceModulo(int x, int y)
{
	int z = x % y;
	if( z >=0 ) {
		return z;
	}
	return z + y;
}

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

// semi can be negative
// returns a note that also incorporates the octave
int semiToNoteWithinKeySig(int semi, int key, std::string modeString, bool *outOfKey)
{
	assert(key >= 0);
	assert(key <= 11);
	assert(modeString[7] == '\0');

	int semiToTry = key;
	*outOfKey = false;
	int semiModulo12 = niceModulo(semi, 12);
 	for( int noteToTry = 0; noteToTry <= 6; noteToTry++ ) {
		if( semiModulo12 == niceModulo(semiToTry, 12) ) {
			// Found a matching note, now find the octave
			for( int octave = -10; octave < 10; octave++ ) {
				if( semi == semiToTry + octave * 12 ) {
					return noteToTry + octave * 7;
				}
			}
		}
		if( modeString[noteToTry] == 'T' ) {
			semiToTry += 2;
		} else {
			semiToTry += 1;
		}
	}
	// No exact match. Find the match that works when we start a semi lower...
	*outOfKey = true;
	semiToTry = key;
	semi -= 1;
	semiModulo12 = niceModulo(semi, 12);
	for( int noteToTry = 0; noteToTry <= 6; noteToTry++ ) {
		if( semiModulo12 == niceModulo(semiToTry, 12) ) {
			// Found a matching note, now find the octave
			for( int octave = -10; octave < 10; octave++ ) {
				if( semi == semiToTry + octave * 12 ) {
					return noteToTry + octave * 7;
				}
			}
		}
		if( modeString[noteToTry] == 'T' ) {
			semiToTry += 2;
		} else {
			semiToTry += 1;
		}
	}
	// That didn't match either. Impossible!
	assert(false);
	return 0;
}

int noteToSemiWithinKeySig(int note, int key, std::string modeString)
{
	int semi = key;
	// First shift by whole octaves
	while( note < 0 ) {
		semi -= 12;
		note += 7;
	}
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
		int semiCalc = noteToSemiWithinKeySig(note, key, modeString);
		assert(semiCalc == semi || outOfKey);
	}
}

std::pair<float, float> primarySecondaryScore(const std::vector<float>& toChordPitches, const std::vector<float>& fromChordPitches)
{
	// For now we insist on the "maximize the minimum absolute difference" jumbling.
	std::vector<float> differences;
	float totalDifference = 0.f;
	for( unsigned int i=0; i<toChordPitches.size(); i++ ) {
		float absDiff = abs(toChordPitches[i] - fromChordPitches[i]);
		differences.push_back(absDiff);
		totalDifference += absDiff;
	}
	float minDiff = *std::min_element(differences.begin(), differences.end());
	return std::make_pair(minDiff, totalDifference);
}

std::vector<float> jumbleChord(std::vector<float> toChordPitches, std::vector<float> fromChordPitches)
{
	INFO("Asserting vectors sizes...");
	assert(toChordPitches.size() == fromChordPitches.size());
	INFO("...Done");

	std::vector<float> candidateToPitches = toChordPitches;
	// Important: sort it first, to get the lexicographically smallest permutation
	std::sort(candidateToPitches.begin(), candidateToPitches.end());

	// We assess the lexicographically first permutation
	std::pair<float, float> bestPrimarySecondaryScore = primarySecondaryScore(candidateToPitches, fromChordPitches);
	// So far (out of a count of one!) it's the best permutation we have
	std::vector<float> bestPermutation = candidateToPitches;

	do {
		// Process current permutation
		std::pair<float,float> currentPrimarySecondaryScore = primarySecondaryScore(candidateToPitches, fromChordPitches);
		if (currentPrimarySecondaryScore.first > bestPrimarySecondaryScore.first
			|| ( (currentPrimarySecondaryScore.first == bestPrimarySecondaryScore.first)
				&& (currentPrimarySecondaryScore.second > bestPrimarySecondaryScore.second) ) ) {
			bestPrimarySecondaryScore = currentPrimarySecondaryScore;
			bestPermutation = candidateToPitches;
				}
	} while (std::next_permutation(candidateToPitches.begin(), candidateToPitches.end()));

	return bestPermutation;
}

void runTests()
{
	assert(voltageToNearestSemi(0.f) == 0);
	assert(voltageToNearestSemi(1.f) == 12);
	assert(voltageToNearestSemi(-1.f) == -12);
	assert(voltageToNearestSemi(1.f / 12.f) == 1);
	assert(voltageToNearestSemi(11.f / 12.f) == 11);

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
	assert(semiToNoteWithinKeySig(9,11,modeString,&outOfKey)==5-7);	// A in B Major Ionian
	assert(outOfKey==true);
	modeString = generateModeString(1);
	assert(semiToNoteWithinKeySig(9,11,modeString,&outOfKey)==6-7);	// A in B Dorian
	assert(outOfKey==false);

	std::vector<float> from;
	std::vector<float> to;
	from.push_back(1.f);
	from.push_back(2.f);
	to.push_back(3.f);
	to.push_back(5.f);
	jumbleChord(from, to);
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

	dsp::SchmittTrigger trigger;		// A handler for the gate input, debouncing kind of thing
	dsp::PulseGenerator pulse;			// A pulse generator for the rollover light
	float prevPitch = 0.0;				// The pitch input from the previous process()
	int prevNumNotes = -1;				// The number of notes in the chord (a param) from the previous process()
	std::vector<float> fromPitches;		// The pitches in the chord we're interpolating from
	std::vector<float> toPitches;		// The pitches in the chord we're intepolating to (guaranteed same size)
	int timerSamples = 0;				// The number of samples that we are "into" a slide
	int timerTarget = 0;				// The time (in samples) that we are expecting to end the slide. 0 means no slide.

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

	std::vector<float> constructChord() {
		int key = (int)(params[KEYSIG_PARAM].getValue());
		int mode = 0;	// Not implemented yet
		std::string modeString = generateModeString(mode);

		float voct = inputs[VOCT_INPUT].getVoltage();
		int semi = voltageToNearestSemi(voct);

		bool outOfKey = false;
		int note = semiToNoteWithinKeySig(semi, key, modeString, &outOfKey);
		// If the user presses a key which is NOT in the selected key signature,
		// note is "forced" to be something nearby in the key signature, and
		// as a "bonus" feature, a first inversion of the chord is played instead
		int firstInversion = outOfKey ? 1 : 0;

		// The user has selected the number of notes in the chord on a knob
		unsigned int numNotes = (int)(params[CHORD_PARAM].getValue());

		// These integers are "nth note within key signature" from pressed root
		std::vector<int> notes;
		for( unsigned int noteIndex = 0; noteIndex < numNotes; noteIndex++ ) {
			int thisNote = note + chordNotes[noteIndex + firstInversion];
			notes.push_back(thisNote);
		}

		// Convert to semitones within key signature
		std::vector<int> semis;
		for( unsigned int noteIndex = 0; noteIndex < numNotes; noteIndex++ ) {
			int thisSemi = noteToSemiWithinKeySig(notes[noteIndex], key, modeString);
			semis.push_back(thisSemi);
		}

		// Comvert to one volt per octave (12 semis in an octave)
		std::vector<float> pitches;
		for( unsigned int noteIndex = 0; noteIndex < numNotes; noteIndex++ ) {
			pitches.push_back((float)(semis[noteIndex]) / 12.f);
		}
		return pitches;
	}

	// Progress is a float between zero (start of slide) and one (end of slide)
	std::vector<float> interpolatePitches(float progress)
	{
		assert(toPitches.size() == fromPitches.size());

		// Interpolate between fromPitches (progress==0) and toPitches (progress==1)
		std::vector<float> pitches;
		for( unsigned int i=0; i<fromPitches.size(); i++ ) {
			float pitch = progress * toPitches[i] + (1.f - progress) * fromPitches[i];
			pitches.push_back(pitch);
		}
		return pitches;
	}

	// Output the polyphonic pitches
	void playChord(std::vector<float> pitches)
	{
		outputs[VOCT_OUTPUT].setChannels(pitches.size());
		for( unsigned int i=0; i<pitches.size(); i++ ) {
			outputs[VOCT_OUTPUT].setVoltage(pitches[i], i);
		}
	}

	void process(const ProcessArgs& args) override {
		// First we check that the number of notes hasn't changed. Otherwise a mixture
		// of sizes for fromPitches and toPitches could be disastrous!
		int numNotes = (int)(params[CHORD_PARAM].getValue());
		bool numNotesChanged = (numNotes != prevNumNotes);
		prevNumNotes = numNotes;

		// Has the input gate been triggered this sample?
		float gateV = inputs[GATE_INPUT].getVoltage();
		auto tc = trigger.processEvent(gateV, 0.1, 1.0);
		bool gateOn = (tc==dsp::SchmittTrigger::TRIGGERED);	// The gate has gone from "unpressed" to "pressed"

		// Has the pitch changed more than 0.5 semitones this sample?
		float pitchV = inputs[VOCT_INPUT].getVoltage();
		bool pitchChange = abs(pitchV - prevPitch) > 1.f / 12.f / 2.f;
		prevPitch = pitchV;

		// Has the pitch changed without a gate trigger?
		// (and we don't want to rollover if the number of notes has changed)
		bool rollover = pitchChange && (!gateOn) && (!numNotesChanged);

		// If so we want to slide. Flash the light for as long as we expect the slide to continue
		if( rollover ) {
			float glidePeriod = params[TIME_PARAM].getValue();
			pulse.trigger(glidePeriod);
		}
		bool light = pulse.process(args.sampleTime);
		lights[ROLLOVER_LIGHT].setBrightness(light ? 1.f : 0.f);

		// The polyphonic gate outputs are just copies of the input gate
		float gate = inputs[GATE_INPUT].getVoltage();
		outputs[GATE_OUTPUT].setChannels(numNotes);
		for( int i=0; i<numNotes; i++ ) {
			outputs[GATE_OUTPUT].setVoltage(gate, i);
		}

		if( gateOn || numNotesChanged ) {
			// For a gateOn trigger, we are starting a chord
			// We also do this when the user moves the "number of notes" knob
			// (in that case, the "new" chord will only sound if the gate input is high, as this is copied to output gates)
			fromPitches = constructChord();
			playChord(fromPitches);
			timerTarget = 0;		// Indicate "no current slide"
		} else if ( rollover ) {
			// User has "rolled over" from one key to another, initiating a portamento glide

			// Where do we want to get to?
			auto chord = constructChord();

			// Where are we now?
			bool abortedPreviousGlide = false;
			if( timerTarget > 0 && timerSamples <= timerTarget ) {
				// We WERE in the middle of a slide already.
				// Start the new slide from wherever we'd got to so far.
				float progress = ((float)(timerSamples)) / timerTarget;
				fromPitches = interpolatePitches(progress);
				abortedPreviousGlide = true;
			} else {
				// We're sliding from a previously steady chord
				// fromPitches is already set
			}

			// Jumble notes in chord according to settings, where we are now, and where we want to get to
			toPitches = jumbleChord(chord, fromPitches);

			// Calculate timerTarget based on glide time setting knob
			float glidePeriodSeconds = params[TIME_PARAM].getValue();
			if( abortedPreviousGlide ) {
				// If we started this glide from within a previous glide, we spend half as long on the new glide,
				// and restart the pulse generator for the rollover light accordingly
				glidePeriodSeconds = glidePeriodSeconds / 2.f;
				pulse.trigger(glidePeriodSeconds);
			}
			float sampleTimeSeconds = args.sampleTime;
			timerTarget = (int)(glidePeriodSeconds / sampleTimeSeconds);	// End of glide

			timerSamples = 0;	// Start of glide
		}

		if( timerTarget==0 ) {
			// No current glide
		} else if( timerSamples < timerTarget ) {
			// Mid glide
			timerSamples++;
			float progress = ((float)(timerSamples)) / timerTarget;
			std::vector<float> pitches = interpolatePitches(progress);
			playChord(pitches);
		} else if( timerSamples == timerTarget ) {
			// End point of glide. Act like this "always was" a flat unglided chord
			fromPitches = toPitches;
			timerTarget = 0;
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