#include "plugin.hpp"


struct ChordRollover : Module {
	enum ParamId {
		EXAMPLE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		EXAMPLE_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		EXAMPLE_OUTPUT,
		VOCT_OUTPUT,
		GATE_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		EXAMPLE_LIGHT,
		LIGHTS_LEN
	};

	ChordRollover() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(EXAMPLE_PARAM, 0.f, 1.f, 0.f, "");
		configInput(EXAMPLE_INPUT, "");
		configOutput(EXAMPLE_OUTPUT, "");
		configOutput(VOCT_OUTPUT, "Pitch");
		configOutput(GATE_OUTPUT, "Gate");
	}

	void process(const ProcessArgs& args) override {
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

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(7.735, 16.277)), module, ChordRollover::EXAMPLE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7.513, 35.36)), module, ChordRollover::EXAMPLE_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7.606, 52.759)), module, ChordRollover::EXAMPLE_OUTPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7, 113)), module, ChordRollover::VOCT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(width - 7, 113)), module, ChordRollover::GATE_OUTPUT));


		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(7.325, 70.813)), module, ChordRollover::EXAMPLE_LIGHT));

		// mm2px(Vec(10.0, 10.0))
		addChild(createWidget<Widget>(mm2px(Vec(2.684, 81.945))));
	}
};


Model* modelChordRollover = createModel<ChordRollover, ChordRolloverWidget>("ChordRollover");