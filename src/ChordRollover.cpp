#include "plugin.hpp"
#include <cfloat>

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

void meanStandardDeviation(const std::vector<float>& data, float &mean, float &stdDev)
{
	float sum = 0.f;
	float sum_of_squares = 0.f;
	unsigned int n = data.size();

	for( unsigned int i = 0; i < n; i++ ) {
		sum += data[i];
		sum_of_squares += (data[i] * data[i]);
	}

	mean = sum / n;
	float variance = (sum_of_squares - (sum * sum / n)) / (n - 1);
	stdDev = sqrt(variance);
}

// A primary and secondary score for comparing "jumbleness" of chord mappings. Biggest score wins.
std::vector<float> primarySecondaryTertiaryScore(const std::vector<float>& toChordPitches, const std::vector<float>& fromChordPitches, bool jumbleMode)
{
	std::vector<float> pitchChange = toChordPitches;
	for( unsigned int i = 0; i < toChordPitches.size(); i++ ) {
		pitchChange[i] -= fromChordPitches[i];
	}

	float meanPitchChange = 0.f;
	float stdDevPitchChange = 0.f;
	meanStandardDeviation(pitchChange, meanPitchChange, stdDevPitchChange);

	float minAbsChange = FLT_MAX;
	for( unsigned int i = 0; i < pitchChange.size(); i++ ) {
		minAbsChange = std::min(minAbsChange, abs(pitchChange[i]));
	}
	bool anyNotesSame = (minAbsChange < 1.f / 12.f / 5.f);	// Difference less than 5th of a semitone

	std::vector<float> result;
	if( jumbleMode ) {
		// It is essential (so far as possible) that no notes stay the same
		result.push_back(anyNotesSame ? 0.f : 1.f);		// Primary score, prefer no notes the same
		// It is intended that notes change by different amounts
		result.push_back(stdDevPitchChange);
		// If further sorting is required, we'd prefer mapppings where the minimum delta is large
		result.push_back(minAbsChange);
	} else {
		// Largely we want the inverse of "jumbling" to be the opposite extreme. So we negate the above
		// BUT we still want to avoid any notes staying the same
		result.push_back(anyNotesSame ? 0.f : 1.f);		// Primary score, still prefer no notes the same; Don't negate
		result.push_back(-stdDevPitchChange);
		result.push_back(-minAbsChange);
	}
	return result;
}

bool vectorABeatsB(std::vector<float> A, std::vector<float> B)
{
	assert( A.size() == B.size() );

	for( unsigned int i = 0; i < A.size(); i++ ) {
		if( A[i] > B[i] ) {
			return true;
		} else if( A[i] < B[i] ) {
			return false;
		}
		// so we have A[i] == B[i]
	}
	return false;	// Vectors are equal
}

std::vector<float> jumbleChord(const std::vector<float> &toChordPitches, const std::vector<float> &fromChordPitches, bool jumbleMode)
{
	assert(toChordPitches.size() == fromChordPitches.size());

	std::vector<float> candidateToPitches = toChordPitches;
	// Important: sort it first, to get the lexicographically smallest permutation
	std::sort(candidateToPitches.begin(), candidateToPitches.end());

	// We assess the lexicographically first permutation
	std::vector<float> bestScore = primarySecondaryTertiaryScore(candidateToPitches, fromChordPitches, jumbleMode);
	// So far (out of a count of one!) it's the best permutation we have
	std::vector<float> bestPermutation = candidateToPitches;

	do {
		// Process current permutation
		std::vector<float> currentPrimarySecondaryScore = primarySecondaryTertiaryScore(candidateToPitches, fromChordPitches, jumbleMode);
		if( vectorABeatsB(currentPrimarySecondaryScore, bestScore) ) {
			bestScore = currentPrimarySecondaryScore;
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
	jumbleChord(from, to, true);
}

int chordNotes[8] = {0,2,4,7,9,11,14,16};	// Room for a heptad plus first inversion

struct ChordRollover : Module {
	enum ParamId {
		KEYSIG_PARAM,
		MODE_PARAM,
		CHORD_PARAM,
		TIME_PARAM,
		PROFILE_PARAM,
		JUMBLE_PARAM,
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

	dsp::SchmittTrigger trigger;		// A handler for the gate input, debouncing kind of thing
	float prevPitch = 0.0;				// The pitch input from the previous process()
	int prevNumNotes = -1;				// The number of notes in the chord (a param) from the previous process()
	std::vector<float> fromPitches;		// The pitches in the chord we're interpolating from
	std::vector<float> toPitches;		// The pitches in the chord we're intepolating to (guaranteed same size)
	int lastValidNotePressed = 0;		// The last valid note pressed (valid in this key sig / mode)
	int timerSamples = 0;				// The number of samples that we are "into" a slide
	int timerTarget = 0;				// The time (in samples) that we are expecting to end the slide. 0 means no slide.
	bool gateSuppressed = false;		// We hold this true from when a new keypress is not in the correct key

	ChordRollover() {
		INFO("Running Tests...");
		runTests();
		INFO("...Tests completed");

		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configSwitch(KEYSIG_PARAM, 0.f, 11.f, 0.f, "Key Signature", {"C", "C♯/D♭", "D", "D♯/E♭", "E", "F", "F♯/G♭", "G", "G♯/A♭", "A", "A♯/B♭", "B"});
		configSwitch(MODE_PARAM, 0.f, 6.f, 0.f, "Mode", {"Ionian (Major)", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Aeolian (Minor)", "Locrian"});
		configSwitch(CHORD_PARAM, 1.f, 7.f, 4.f, "Notes in chord", {"Monad", "Diad", "Triad", "Tetrad", "Pentad", "Hexad", "Heptad"});
		configParam(TIME_PARAM, 0.001f, 5.f, 0.5f, "Glide time (s)" );
		configParam(PROFILE_PARAM, 1.f, 10.f, 1.f, "Glide profile (1 for triangle, 10 for step)");
		configSwitch(JUMBLE_PARAM, 0.f, 1.f, 0.f, "Jumble Mode", {"Off", "On"});
		configInput(VOCT_INPUT, "(Mono) Pitch");
		configInput(GATE_INPUT, "(Mono) Gate");
		configOutput(VOCT_OUTPUT, "(Poly) Pitch");
		configOutput(GATE_OUTPUT, "(Poly) Gate");
	}

	int notePressed(bool &invalidPress) {
		int key = (int)(params[KEYSIG_PARAM].getValue());
		int mode = (int)(params[MODE_PARAM].getValue());
		std::string modeString = generateModeString(mode);

		float voct = inputs[VOCT_INPUT].getVoltage();
		int semi = voltageToNearestSemi(voct);

		return semiToNoteWithinKeySig(semi, key, modeString, &invalidPress);
	}

	std::vector<float> constructChord(int validNote) {
		int key = (int)(params[KEYSIG_PARAM].getValue());
		int mode = (int)(params[MODE_PARAM].getValue());
		std::string modeString = generateModeString(mode);

		// The user has selected the number of notes in the chord on a knob
		unsigned int numNotes = (int)(params[CHORD_PARAM].getValue());

		// These integers are "nth note within key signature" from pressed root
		std::vector<int> notes;
		for( unsigned int noteIndex = 0; noteIndex < numNotes; noteIndex++ ) {
			int thisNote = validNote + chordNotes[noteIndex];
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

		// Here is the profile that goes from 0 to 1 as the input parameter goes from 0 to 1
		// (This is the "nth order algebraic sigmoid", used as a parametric smoothstep function, with n as squareness).
		// n = 1 gives triangle (minimum squareness) and n=infinity would give a square step at x=0.5.

		/* Old method, symmetric in time
		float n = params[PROFILE_PARAM].getValue();
		float x = progress;
		// y = x^n/(x^n + (1-x)^n)
		float xToN = pow(x,n);
		float y = xToN / (xToN + pow(1.f - x,n));
		float modifiedProgress = y;*/

		// New code, with sharp start and soft end
		float n = params[PROFILE_PARAM].getValue();
		float x = (progress/2.f + 0.5);		// x now goes from 0.5 to 1.0
		// y = x^n/(x^n + (1-x)^n)
		float xToN = pow(x,n);
		float y = xToN / (xToN + pow(1.f - x,n));
		// y at this point goes from 0.5 to 1.0
		// map y to (0.0 to 1.0)
		y = (y-0.5f) * 2.f;
		float modifiedProgress = y;

		// Interpolate between fromPitches (progress==0) and toPitches (progress==1)
		std::vector<float> pitches;
		for( unsigned int i=0; i<fromPitches.size(); i++ ) {
			float pitch = modifiedProgress * toPitches[i] + (1.f - modifiedProgress) * fromPitches[i];
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

		// Is the current keypress pitch supported in the current key signature / mode?
		bool invalidPress = false;
		int note = notePressed(invalidPress);
		if( !invalidPress ) {
			lastValidNotePressed = note;
			gateSuppressed = false;
		}
		std::vector<float> pitches = constructChord(lastValidNotePressed);

		// Has the input gate been triggered this sample?
		float gateV = inputs[GATE_INPUT].getVoltage();
		auto tc = trigger.processEvent(gateV, 0.1, 1.0);
		bool gateOn = (tc==dsp::SchmittTrigger::TRIGGERED);	// The gate has gone from "unpressed" to "pressed"
		bool pitchChange = false;
		if( invalidPress ) {
			// The user pressed a key that's not in this key signature / mode. Act like it didn't happen
			if( gateOn ) {
				// It was a new keypress. Get rid of it altogether
				gateOn = false;
				// And supress the output gates for a while
				gateSuppressed = true;
			} else {
				// It was a rollover to an invalid key
				// gateOn is already false
				// Leave gateV as it is, because it still relates to the key being rolled from
			}
		} else {
			// Has the pitch changed more than 0.5 semitones this sample?
			float pitchV = inputs[VOCT_INPUT].getVoltage();
			pitchChange = abs(pitchV - prevPitch) > 1.f / 12.f / 2.f;
			prevPitch = pitchV;
		}

		// Has the pitch changed without a gate trigger?
		// (and we don't want to rollover if the number of notes has changed)
		bool rollover = pitchChange && (!gateOn) && (!numNotesChanged);

		// The polyphonic gate outputs are just copies of the input gate (unless suppressed by an out-of-key press)
		if( gateSuppressed ) {
			gateV = 0.f;
		}
		outputs[GATE_OUTPUT].setChannels(numNotes);
		for( int i=0; i<numNotes; i++ ) {
			outputs[GATE_OUTPUT].setVoltage(gateV, i);
		}

		if( (gateOn) || numNotesChanged ) {
			// For a gateOn trigger, we are starting a chord
			// We also do this when the user moves the "number of notes" knob
			// (in that case, the "new" chord will only sound if the gate input is high, as this is copied to output gates)
			fromPitches = pitches;
			playChord(fromPitches);
			timerTarget = 0;		// Indicate "no current slide"
			lights[ROLLOVER_LIGHT].setBrightness(0.f);
		} else if ( rollover ) {
			// User has "rolled over" from one key to another, initiating a portamento glide

			// Where do we want to get to?
			auto chord = pitches;

			// Where are we now?
			bool abortedPreviousGlide = false;
			if( timerTarget > 0 && timerSamples <= timerTarget ) {
				// We WERE in the middle of a slide already.
				// Start the new slide from wherever we'd got to so far.
				float progress = ((float)(timerSamples)) / timerTarget;
				fromPitches = interpolatePitches(progress);
				std::sort(fromPitches.begin(), fromPitches.end());	// Seems sensible in the face of a slight bug
				abortedPreviousGlide = true;
			} else {
				// We're sliding from a previously steady chord
				// fromPitches is already set
			}

			// Jumble notes in chord according to: jumble mode, where we are now, and where we want to get to
			bool jumbleMode = (params[JUMBLE_PARAM].getValue() == 1.f);
			toPitches = jumbleChord(chord, fromPitches, jumbleMode);

			// Calculate timerTarget based on glide time setting knob
			float glidePeriodSeconds = params[TIME_PARAM].getValue();
			if( abortedPreviousGlide ) {
				// If we started this glide from within a previous glide, we spend half as long on the new glide,
				glidePeriodSeconds = glidePeriodSeconds / 2.f;
			}
			float sampleTimeSeconds = args.sampleTime;
			timerTarget = (int)(glidePeriodSeconds / sampleTimeSeconds);	// When the end of glide will be

			timerSamples = 0;	// Start of glide
		}

		if( timerTarget==0 ) {
			// No current glide
			lights[ROLLOVER_LIGHT].setBrightness(0.f);
		} else if( timerSamples < timerTarget ) {
			// Mid glide
			timerSamples++;
			float progress = ((float)(timerSamples)) / timerTarget;
			std::vector<float> interPitches = interpolatePitches(progress);
			playChord(interPitches);
			// Light up the "Rollover" LED in a kinda linear way based on progress instead of a fixed length pulse
			lights[ROLLOVER_LIGHT].setBrightness(1.f/3.f + 2.f * (1.f - progress)/3.f);
		} else if( timerSamples == timerTarget ) {
			// End point of glide. From now, act like this "always was" a flat unglided chord
			fromPitches = toPitches;
			std::sort(fromPitches.begin(), fromPitches.end());	// Seems sensible in the face of a slight bug
			timerTarget = 0;
		}

	}
};

struct ChordRolloverWidget : ModuleWidget {
	ChordRolloverWidget(ChordRollover* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Panel.svg")));

		float width = 5.08 * 6;
		float col1 = 5.08 * 1.5;
		float col2 = 5.08 * 4.5;

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(col1, 25)), module, ChordRollover::KEYSIG_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(col2, 25)), module, ChordRollover::MODE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(col1, 50)), module, ChordRollover::CHORD_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(col2, 50)), module, ChordRollover::JUMBLE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(col1, 75)), module, ChordRollover::PROFILE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(col2, 75)), module, ChordRollover::TIME_PARAM));

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