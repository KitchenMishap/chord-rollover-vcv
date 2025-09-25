#include "plugin.hpp"


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