#include "plugin.hpp"

void runTests()
{
	assert(true);
}


struct ToDo : Module {
	enum ParamId {
		TODO_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TODO_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		TODO_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		TODO_LIGHT,
		LIGHTS_LEN
	};

	ToDo() {
		INFO("ToDo: Running Tests...");
		runTests();
		INFO("ToDo: ...Tests completed");

		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		//configSwitch(TODO_PARAM, 0.f, 2.f, 0.f, "ToDo Param", {"0.0", "1.0", "2.0"});
		configParam(TODO_PARAM, 0.f, 5.f, 1.f, "ToDo Param" );
		configInput(TODO_INPUT, "ToDo Input");
		configOutput(TODO_OUTPUT, "ToDo Output");
	}


	void process(const ProcessArgs& args) override {
		// Example: Set polyphonic output equal to input
		int numPoly = inputs[TODO_INPUT].getChannels();
		outputs[TODO_OUTPUT].setChannels(numPoly);
		for( int i=0; i<numPoly; i++ ) {
			float inputV = inputs[TODO_INPUT].getVoltage(i);
			outputs[TODO_OUTPUT].setVoltage(inputV, i);
		}
		// Example: Turn LED on
		lights[TODO_LIGHT].setBrightness(1.f);
	}
};

struct ToDoWidget : ModuleWidget {
	ToDoWidget(ToDo* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/GeneratedPanelPaths.svg")));

		float width = 5.08 * 6;
		float col1 = 5.08 * 1.5;
		//float col2 = 5.08 * 4.5;

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(col1, 25)), module, ToDo::TODO_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(7, 97)), module, ToDo::TODO_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(7, 113)), module, ToDo::TODO_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(width - 7, 86)), module, ToDo::TODO_LIGHT));

		// mm2px(Vec(10.0, 10.0))
		addChild(createWidget<Widget>(mm2px(Vec(2.684, 81.945))));
	}
};

Model* modelToDo = createModel<ToDo, ToDoWidget>("ToDo");