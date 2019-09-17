//***********************************************************************************************
//Mixer module for VCV Rack by Steve Baker and Marc Boulé 
//
//Based on code from the Fundamental plugin by Andrew Belt 
//See ./LICENSE.txt for all licenses
//***********************************************************************************************


#include "MixerWidgets.hpp"


struct AuxExpander : Module {
	enum ParamIds {
		ENUMS(TRACK_AUXSEND_PARAMS, 16 * 4), // trk 1 aux A, trk 1 aux B, ... 
		ENUMS(GROUP_AUXSEND_PARAMS, 4 * 4),// must be contiguous with TRACK_AUXSEND_PARAMS
		ENUMS(TRACK_AUXMUTE_PARAMS, 16),
		ENUMS(GROUP_AUXMUTE_PARAMS, 4),// must be contiguous with TRACK_AUXMUTE_PARAMS
		ENUMS(GLOBAL_AUXSEND_PARAMS, 4),
		ENUMS(GLOBAL_AUXPAN_PARAMS, 4),
		ENUMS(GLOBAL_AUXRETURN_PARAMS, 4),// must be contiguous with GLOBAL_AUXPAN_PARAMS
		ENUMS(GLOBAL_AUXMUTE_PARAMS, 4),// must be contiguous with GLOBAL_AUXRETURN_PARAMS
		ENUMS(GLOBAL_AUXSOLO_PARAMS, 4),// must be contiguous with GLOBAL_AUXMUTE_PARAMS
		ENUMS(GLOBAL_AUXGROUP_PARAMS, 4),// must be contiguous with GLOBAL_AUXSOLO_PARAMS
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(RETURN_INPUTS, 2 * 4),
		ENUMS(POLY_AUX_AD_CV_INPUTS, 4),
		POLY_AUX_M_CV_INPUT,
		POLY_GRPS_AD_CV_INPUT,
		POLY_GRPS_M_CV_INPUT,
		POLY_BUS_CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(SEND_OUTPUTS, 2 * 4),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	// Expander
	float leftMessages[2][AFM_NUM_VALUES] = {};// messages from mother (first index is page), see enum called AuxFromMotherIds in Mixer.hpp


	// Constants
	// none


	// Need to save, no reset
	int panelTheme;
	int8_t vuColorThemeLocal[4]; // 0 to numthemes - 1; (when per-track choice)
	
	
	// Need to save, with reset
	
	
	// No need to save, with reset
	int refreshCounter80;
	VuMeterAllDual vu[4];
	float globalSends[4];
	float mutes[20];
	
	
	// No need to save, no reset
	bool motherPresent = false;// can't be local to process() since widget must know in order to properly draw border
	alignas(4) char trackLabels[4 * 20 + 1] = "-01--02--03--04--05--06--07--08--09--10--11--12--13--14--15--16-GRP1GRP2GRP3GRP4";// 4 chars per label, 16 tracks and 4 groups means 20 labels, null terminate the end the whole array only
	ColorAndCloak colorAndCloak;
	int updateTrackLabelRequest = 0;// 0 when nothing to do, 1 for read names in widget
	int resetAuxLabelRequest = 0;// 0 when nothing to do, 1 for reset names in widget	
	
	
	AuxExpander() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);		
		
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];
		
		char strBuf[32];

		float maxAGIndivSendFader = std::pow(GlobalInfo::individualAuxSendMaxLinearGain, 1.0f / GlobalInfo::individualAuxSendScalingExponent);
		for (int i = 0; i < 16; i++) {
			// Track send aux A
			snprintf(strBuf, 32, "Track #%i aux send A", i + 1);
			configParam(TRACK_AUXSEND_PARAMS + i * 4 + 0, 0.0f, maxAGIndivSendFader, 0.0f, strBuf, " dB", -10, 20.0f * GlobalInfo::individualAuxSendScalingExponent);
			// Track send aux B
			snprintf(strBuf, 32, "Track #%i aux send B", i + 1);
			configParam(TRACK_AUXSEND_PARAMS + i * 4 + 1, 0.0f, maxAGIndivSendFader, 0.0f, strBuf, " dB", -10, 20.0f * GlobalInfo::individualAuxSendScalingExponent);
			// Track send aux C
			snprintf(strBuf, 32, "Track #%i aux send C", i + 1);
			configParam(TRACK_AUXSEND_PARAMS + i * 4 + 2, 0.0f, maxAGIndivSendFader, 0.0f, strBuf, " dB", -10, 20.0f * GlobalInfo::individualAuxSendScalingExponent);
			// Track send aux D
			snprintf(strBuf, 32, "Track #%i aux send D", i + 1);
			configParam(TRACK_AUXSEND_PARAMS + i * 4 + 3, 0.0f, maxAGIndivSendFader, 0.0f, strBuf, " dB", -10, 20.0f * GlobalInfo::individualAuxSendScalingExponent);
			// Mute
			snprintf(strBuf, 32, "Track #%i aux send mute", i + 1);
			configParam(TRACK_AUXMUTE_PARAMS + i, 0.0f, 1.0f, 0.0f, strBuf);
		}
		for (int i = 0; i < 4; i++) {
			// Group send aux A
			snprintf(strBuf, 32, "Group #%i aux send A", i + 1);
			configParam(GROUP_AUXSEND_PARAMS + i * 4 + 0, 0.0f, maxAGIndivSendFader, 0.0f, strBuf, " dB", -10, 20.0f * GlobalInfo::individualAuxSendScalingExponent);
			// Group send aux B
			snprintf(strBuf, 32, "Group #%i aux send B", i + 1);
			configParam(GROUP_AUXSEND_PARAMS + i * 4 + 1, 0.0f, maxAGIndivSendFader, 0.0f, strBuf, " dB", -10, 20.0f * GlobalInfo::individualAuxSendScalingExponent);
			// Group send aux C
			snprintf(strBuf, 32, "Group #%i aux send C", i + 1);
			configParam(GROUP_AUXSEND_PARAMS + i * 4 + 2, 0.0f, maxAGIndivSendFader, 0.0f, strBuf, " dB", -10, 20.0f * GlobalInfo::individualAuxSendScalingExponent);
			// Group send aux D
			snprintf(strBuf, 32, "Group #%i aux send D", i + 1);
			configParam(GROUP_AUXSEND_PARAMS + i * 4 + 3, 0.0f, maxAGIndivSendFader, 0.0f, strBuf, " dB", -10, 20.0f * GlobalInfo::individualAuxSendScalingExponent);
			// Mute
			snprintf(strBuf, 32, "Group #%i aux send mute", i + 1);
			configParam(GROUP_AUXMUTE_PARAMS + i, 0.0f, 1.0f, 0.0f, strBuf);		
		}

		// Global send aux A-D
		float maxAGGlobSendFader = std::pow(GlobalInfo::globalAuxSendMaxLinearGain, 1.0f / GlobalInfo::globalAuxSendScalingExponent);
		configParam(GLOBAL_AUXSEND_PARAMS + 0, 0.0f, maxAGGlobSendFader, 1.0f, "Global aux send A", " dB", -10, 20.0f * GlobalInfo::globalAuxSendScalingExponent);
		configParam(GLOBAL_AUXSEND_PARAMS + 1, 0.0f, maxAGGlobSendFader, 1.0f, "Global aux send B", " dB", -10, 20.0f * GlobalInfo::globalAuxSendScalingExponent);
		configParam(GLOBAL_AUXSEND_PARAMS + 2, 0.0f, maxAGGlobSendFader, 1.0f, "Global aux send C", " dB", -10, 20.0f * GlobalInfo::globalAuxSendScalingExponent);
		configParam(GLOBAL_AUXSEND_PARAMS + 3, 0.0f, maxAGGlobSendFader, 1.0f, "Global aux send D", " dB", -10, 20.0f * GlobalInfo::globalAuxSendScalingExponent);

		// Global pan return aux A-D
		configParam(GLOBAL_AUXPAN_PARAMS + 0, 0.0f, 1.0f, 0.5f, "Global aux return pan A", "%", 0.0f, 200.0f, -100.0f);
		configParam(GLOBAL_AUXPAN_PARAMS + 1, 0.0f, 1.0f, 0.5f, "Global aux return pan B", "%", 0.0f, 200.0f, -100.0f);
		configParam(GLOBAL_AUXPAN_PARAMS + 2, 0.0f, 1.0f, 0.5f, "Global aux return pan C", "%", 0.0f, 200.0f, -100.0f);
		configParam(GLOBAL_AUXPAN_PARAMS + 3, 0.0f, 1.0f, 0.5f, "Global aux return pan D", "%", 0.0f, 200.0f, -100.0f);

		// Global return aux A-D
		float maxAGAuxRetFader = std::pow(GlobalInfo::globalAuxReturnMaxLinearGain, 1.0f / GlobalInfo::globalAuxReturnScalingExponent);
		configParam(GLOBAL_AUXRETURN_PARAMS + 0, 0.0f, maxAGAuxRetFader, 1.0f, "Global aux return A", " dB", -10, 20.0f * GlobalInfo::globalAuxReturnScalingExponent);
		configParam(GLOBAL_AUXRETURN_PARAMS + 1, 0.0f, maxAGAuxRetFader, 1.0f, "Global aux return B", " dB", -10, 20.0f * GlobalInfo::globalAuxReturnScalingExponent);
		configParam(GLOBAL_AUXRETURN_PARAMS + 2, 0.0f, maxAGAuxRetFader, 1.0f, "Global aux return C", " dB", -10, 20.0f * GlobalInfo::globalAuxReturnScalingExponent);
		configParam(GLOBAL_AUXRETURN_PARAMS + 3, 0.0f, maxAGAuxRetFader, 1.0f, "Global aux return D", " dB", -10, 20.0f * GlobalInfo::globalAuxReturnScalingExponent);

		// Global mute
		configParam(GLOBAL_AUXMUTE_PARAMS + 0, 0.0f, 1.0f, 0.0f, "Global aux send mute A");		
		configParam(GLOBAL_AUXMUTE_PARAMS + 1, 0.0f, 1.0f, 0.0f, "Global aux send mute B");		
		configParam(GLOBAL_AUXMUTE_PARAMS + 2, 0.0f, 1.0f, 0.0f, "Global aux send mute C");		
		configParam(GLOBAL_AUXMUTE_PARAMS + 3, 0.0f, 1.0f, 0.0f, "Global aux send mute D");		

		// Global solo
		configParam(GLOBAL_AUXSOLO_PARAMS + 0, 0.0f, 1.0f, 0.0f, "Global aux send solo A");		
		configParam(GLOBAL_AUXSOLO_PARAMS + 1, 0.0f, 1.0f, 0.0f, "Global aux send solo B");		
		configParam(GLOBAL_AUXSOLO_PARAMS + 2, 0.0f, 1.0f, 0.0f, "Global aux send solo C");		
		configParam(GLOBAL_AUXSOLO_PARAMS + 3, 0.0f, 1.0f, 0.0f, "Global aux send solo D");		


		colorAndCloak.cc1 = 0;
		
		onReset();

		panelTheme = 0;//(loadDarkAsDefault() ? 1 : 0);

	}
  
	void onReset() override {
		for (int i = 0; i < 4; i++) {
			vuColorThemeLocal[i] = 0;
		}
		resetAuxLabelRequest = 1;
		resetNonJson(false);
	}
	void resetNonJson(bool recurseNonJson) {
		refreshCounter80 = 0;
		for (int i = 0; i < 4; i++) {
			vu[i].reset();
			globalSends[i] = 0.0f;
		}
		for (int i = 0; i < 20; i++) {
			mutes[i] = 0.0f;
		}
	}


	void onRandomize() override {
	}

	
	json_t *dataToJson() override {
		json_t *rootJ = json_object();

		// panelTheme
		json_object_set_new(rootJ, "panelTheme", json_integer(panelTheme));

		// vuColorTheme
		std::string buf = "vuColorTheme0";
		for (int i = 0; i < 4; i++ ) {
			json_object_set_new(rootJ, buf.c_str(), json_integer(vuColorThemeLocal[i]));
			buf[12]++;
		}
		
		return rootJ;
	}


	void dataFromJson(json_t *rootJ) override {
		// panelTheme
		json_t *panelThemeJ = json_object_get(rootJ, "panelTheme");
		if (panelThemeJ)
			panelTheme = json_integer_value(panelThemeJ);

		// vuColorTheme
		std::string buf = "vuColorTheme0";
		for (int i = 0; i < 4; i++ ) {
			json_t *vuColorThemeJ = json_object_get(rootJ, buf.c_str());
			if (vuColorThemeJ)
				vuColorThemeLocal[i] = json_integer_value(vuColorThemeJ);
			buf[12]++;
		}
		
		resetNonJson(true);
	}



	void process(const ProcessArgs &args) override {
		motherPresent = (leftExpander.module && leftExpander.module->model == modelMixMaster);
		float *messagesFromMother = (float*)leftExpander.consumerMessage;
		 
		if (motherPresent) {
			// From Mother
			// ***********
			
			// Slow values from mother
			uint32_t* updateSlow = (uint32_t*)(&messagesFromMother[AFM_UPDATE_SLOW]);
			if (*updateSlow != 0) {
				// Track labels
				memcpy(trackLabels, &messagesFromMother[AFM_TRACK_GROUP_NAMES], 4 * 20);
				updateTrackLabelRequest = 1;
				// Panel theme
				int32_t tmp;
				memcpy(&tmp, &messagesFromMother[AFM_PANEL_THEME], 4);
				panelTheme = tmp;
				// Color theme
				memcpy(&colorAndCloak.cc1, &messagesFromMother[AFM_COLOR_AND_CLOAK], 4);
			}
			
			// Fast values from mother
			// Vus handled below outside the block
						
			// Aux sends
			for (int i = 0; i < 8; i++) {
				outputs[SEND_OUTPUTS + i].setVoltage(messagesFromMother[AFM_AUX_SENDS + i]);// left A, right A, left B, right B, left C, right C, left D, right D
			}			
			
			
			// To Mother
			// ***********
			
			// Aux returns
			float *messagesToMother = (float*)leftExpander.module->rightExpander.producerMessage;
			for (int i = 0; i < 8; i++) {
				messagesToMother[AFM_AUX_RETURNS + i] = inputs[RETURN_INPUTS + i].getVoltage();// left A, right A, left B, right B, left C, right C, left D, right D
			}
			leftExpander.module->rightExpander.messageFlipRequested = true;

			// values send 80 (one at a time)
			messagesToMother[AFM_VALUE80_INDEX] = (float)refreshCounter80;
			//   precalc global send knobs with cvs (4 instances)
			if (refreshCounter80 % 20 == 0) {
				int global4i = refreshCounter80 / 20;
				float val = params[GLOBAL_AUXSEND_PARAMS + global4i].getValue();
				val = std::pow(val, GlobalInfo::globalAuxSendScalingExponent);
				if (inputs[POLY_BUS_CV_INPUT].isConnected()) {
					val *= clamp(inputs[POLY_BUS_CV_INPUT].getVoltage(global4i) * 0.1f, 0.0f, 1.0f);
				}
				globalSends[global4i] = val;
			}
			//   precalc trk/grp mutes with cvs (20 instances)
			int global20i = refreshCounter80 / 4;
			if (refreshCounter80 % 4 == 0) {
				float val = params[TRACK_AUXMUTE_PARAMS + global20i].getValue();
				if (global20i < 16) {
					val += inputs[POLY_AUX_M_CV_INPUT].getVoltage(global20i) / 10.0f;
				}
				else {
					val += inputs[POLY_GRPS_M_CV_INPUT].getVoltage(global20i - 16) / 10.0f;
				}
				mutes[global20i] = clamp(1.0f - val, 0.0f, 1.0f);
			}
			//   calc an 80 send value
			float val = params[TRACK_AUXSEND_PARAMS + refreshCounter80].getValue();
			val = std::pow(val, GlobalInfo::individualAuxSendScalingExponent);
			val *= globalSends[refreshCounter80 & 0x3] * mutes[global20i];
			if (refreshCounter80 < 64) {
				int inputNum = POLY_AUX_AD_CV_INPUTS + (refreshCounter80 >> 4);
				if (inputs[inputNum].isConnected()) {
					val *= clamp(inputs[inputNum].getVoltage(refreshCounter80 & 0xF) * 0.1f, 0.0f, 1.0f);
				}
			}
			else {
				if (inputs[POLY_GRPS_AD_CV_INPUT].isConnected()) {
					val *= clamp(inputs[POLY_GRPS_AD_CV_INPUT].getVoltage(refreshCounter80 & 0xF) * 0.1f, 0.0f, 1.0f);
				}
			}			
			messagesToMother[AFM_VALUE80] = val;
			
			// values ret 20 (pan, fader, mute, solo, group) (one of the twenty at a time)
			int refreshCounter20 = refreshCounter80 % 20;
			messagesToMother[AFM_VALUE20_INDEX] = (float)refreshCounter20;
			val = params[GLOBAL_AUXPAN_PARAMS + refreshCounter20].getValue();
			if (refreshCounter20 < 4) {// pan
				val += clamp(inputs[POLY_BUS_CV_INPUT].getVoltage(4 + refreshCounter20), -5.0f, 5.0f) * 0.1f;// this is a -5V to +5V input
			}
			else if (refreshCounter20 < 8) {// fader
				val = std::pow(val, GlobalInfo::globalAuxReturnScalingExponent);
				if (inputs[POLY_BUS_CV_INPUT].isConnected()) {
					val *= clamp(inputs[POLY_BUS_CV_INPUT].getVoltage(4 + refreshCounter20) * 0.1f, 0.f, 1.f);
				}
			}
			else if (refreshCounter20 < 12) {// mute
				val += clamp(inputs[POLY_BUS_CV_INPUT].getVoltage(4 + refreshCounter20) * 0.1f, 0.0f, 1.0f);
			}
			// no CV inputs for solo and group
			messagesToMother[AFM_VALUE20] = val;
			
			refreshCounter80++;
			if (refreshCounter80 >= 80) {
				refreshCounter80 = 0;
			}
		}	

		// VUs
		if (!motherPresent || colorAndCloak.cc4[cloakedMode]) {
			for (int i = 0; i < 4; i++) {
				vu[i].reset();
			}
		}
		else {
			for (int i = 0; i < 4; i++) { 
				vu[i].process(args.sampleTime, &messagesFromMother[AFM_AUX_VUS + (i << 1) + 0]);
			}
		}
		
	}// process()
};


struct AuxExpanderWidget : ModuleWidget {
	PanelBorder* panelBorder;
	bool oldMotherPresent = false;
	TrackAndGroupLabel* trackAndGroupLabels[20];
	AuxDisplay* auxDisplays[4];

	std::string getInitAuxLabel(int aux) {
		std::string ret = "AUXA";
		ret[3] += aux;
		return ret;
	}


	AuxExpanderWidget(AuxExpander *module) {
		setModule(module);

		// Main panels from Inkscape
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/auxspander.svg")));
		panelBorder = findBorder(panel);


		// Left side (globals)
		for (int i = 0; i < 4; i++) {
			// Labels
			addChild(auxDisplays[i] = createWidgetCentered<AuxDisplay>(mm2px(Vec(6.35 + 12.7 * i, 4.7))));
			if (module) {
				auxDisplays[i]->colorAndCloak = &(module->colorAndCloak);
				auxDisplays[i]->srcColor = &(module->vuColorThemeLocal[i]);
				auxDisplays[i]->auxNumber = i;
				auxDisplays[i]->text = getInitAuxLabel(i);
			}
			// Y is 4.7, same X as below
			
			// Left sends
			addOutput(createDynamicPortCentered<DynPort>(mm2px(Vec(6.35 + 12.7 * i, 12.8)), false, module, AuxExpander::SEND_OUTPUTS + i * 2 + 0, module ? &module->panelTheme : NULL));			
			// Right sends
			addOutput(createDynamicPortCentered<DynPort>(mm2px(Vec(6.35 + 12.7 * i, 21.8)), false, module, AuxExpander::SEND_OUTPUTS + i * 2 + 1, module ? &module->panelTheme : NULL));

			// Left returns
			addInput(createDynamicPortCentered<DynPort>(mm2px(Vec(6.35 + 12.7 * i, 31.5)), true, module, AuxExpander::RETURN_INPUTS + i * 2 + 0, module ? &module->panelTheme : NULL));			
			// Right returns
			addInput(createDynamicPortCentered<DynPort>(mm2px(Vec(6.35 + 12.7 * i, 40.5)), true, module, AuxExpander::RETURN_INPUTS + i * 2 + 1, module ? &module->panelTheme : NULL));			
			
			// Pan knobs
			DynSmallKnobGreyWithPanCol *panKnobAux;
			addParam(panKnobAux = createDynamicParamCentered<DynSmallKnobGreyWithPanCol>(mm2px(Vec(6.35 + 12.7 * i, 62.83)), module, AuxExpander::GLOBAL_AUXPAN_PARAMS + i, module ? &module->panelTheme : NULL));
			if (module) {
				panKnobAux->dispColorPtr = &(module->colorAndCloak.cc4[dispColor]);
			}
			
			// Return faders
			addParam(createDynamicParamCentered<DynSmallerFader>(mm2px(Vec(6.35 + 3.67 + 12.7 * i, 87.2)), module, AuxExpander::GLOBAL_AUXRETURN_PARAMS + i, module ? &module->panelTheme : NULL));
			if (module) {
				// VU meters
				VuMeterAux *newVU = createWidgetCentered<VuMeterAux>(mm2px(Vec(6.35 + 12.7 * i, 87.2)));
				newVU->srcLevels = &(module->vu[i]);
				newVU->colorThemeGlobal = &(module->colorAndCloak.cc4[vuColor]);
				newVU->colorThemeLocal = &(module->vuColorThemeLocal[i]);
				addChild(newVU);
			}				
			
			// Global mute buttons
			addParam(createDynamicParamCentered<DynMuteButton>(mm2px(Vec(6.35  + 12.7 * i, 109.8)), module, AuxExpander::GLOBAL_AUXMUTE_PARAMS + i, module ? &module->panelTheme : NULL));
			
			// Global solo buttons
			addParam(createDynamicParamCentered<DynSoloButton>(mm2px(Vec(6.35  + 12.7 * i, 116.1)), module, AuxExpander::GLOBAL_AUXSOLO_PARAMS + i, module ? &module->panelTheme : NULL));		

			// Group dec
			DynGroupMinusButtonNotify *newGrpMinusButton;
			addChild(newGrpMinusButton = createDynamicWidgetCentered<DynGroupMinusButtonNotify>(mm2px(Vec(6.35 - 3.73 + 12.7 * i - 0.75, 123.1)), module ? &module->panelTheme : NULL));
			if (module) {
				newGrpMinusButton->sourceParam = &(module->params[AuxExpander::GLOBAL_AUXGROUP_PARAMS + i]);
			}
			// Group inc
			DynGroupPlusButtonNotify *newGrpPlusButton;
			addChild(newGrpPlusButton = createDynamicWidgetCentered<DynGroupPlusButtonNotify>(mm2px(Vec(6.35 + 3.77 + 12.7 * i + 0.75, 123.1)), module ? &module->panelTheme : NULL));
			if (module) {
				newGrpPlusButton->sourceParam = &(module->params[AuxExpander::GLOBAL_AUXGROUP_PARAMS + i]);
			}
			// Group select displays
			GroupSelectDisplay* groupSelectDisplay;
			addParam(groupSelectDisplay = createParamCentered<GroupSelectDisplay>(mm2px(Vec(6.35 + 12.7 * i - 0.1, 123.1)), module, AuxExpander::GLOBAL_AUXGROUP_PARAMS + i));
			if (module) {
				groupSelectDisplay->srcColor = &(module->colorAndCloak);
			}
		}

		// Global send knobs
		addParam(createDynamicParamCentered<DynSmallKnobAuxA>(mm2px(Vec(6.35 + 12.7 * 0, 51.8)), module, AuxExpander::GLOBAL_AUXSEND_PARAMS + 0, module ? &module->panelTheme : NULL));
		addParam(createDynamicParamCentered<DynSmallKnobAuxB>(mm2px(Vec(6.35 + 12.7 * 1, 51.8)), module, AuxExpander::GLOBAL_AUXSEND_PARAMS + 1, module ? &module->panelTheme : NULL));
		addParam(createDynamicParamCentered<DynSmallKnobAuxC>(mm2px(Vec(6.35 + 12.7 * 2, 51.8)), module, AuxExpander::GLOBAL_AUXSEND_PARAMS + 2, module ? &module->panelTheme : NULL));
		addParam(createDynamicParamCentered<DynSmallKnobAuxD>(mm2px(Vec(6.35 + 12.7 * 3, 51.8)), module, AuxExpander::GLOBAL_AUXSEND_PARAMS + 3, module ? &module->panelTheme : NULL));


		// Right side (individual tracks)
		for (int i = 0; i < 8; i++) {
			// Labels for tracks 1 to 8
			addChild(trackAndGroupLabels[i] = createWidgetCentered<TrackAndGroupLabel>(mm2px(Vec(67.31 + 12.7 * i, 4.7))));
			if (module) {
				trackAndGroupLabels[i]->dispColor = &(module->colorAndCloak.cc4[dispColor]);
			}
			// aux A send for tracks 1 to 8
			addParam(createDynamicParamCentered<DynSmallKnobAuxA>(mm2px(Vec(67.31 + 12.7 * i, 14)), module, AuxExpander::TRACK_AUXSEND_PARAMS + i * 4 + 0, module ? &module->panelTheme : NULL));			
			// aux B send for tracks 1 to 8
			addParam(createDynamicParamCentered<DynSmallKnobAuxB>(mm2px(Vec(67.31 + 12.7 * i, 24.85)), module, AuxExpander::TRACK_AUXSEND_PARAMS + i * 4 + 1, module ? &module->panelTheme : NULL));
			// aux C send for tracks 1 to 8
			addParam(createDynamicParamCentered<DynSmallKnobAuxC>(mm2px(Vec(67.31 + 12.7 * i, 35.7)), module, AuxExpander::TRACK_AUXSEND_PARAMS + i * 4 + 2, module ? &module->panelTheme : NULL));
			// aux D send for tracks 1 to 8
			addParam(createDynamicParamCentered<DynSmallKnobAuxD>(mm2px(Vec(67.31 + 12.7 * i, 46.55)), module, AuxExpander::TRACK_AUXSEND_PARAMS + i * 4 + 3, module ? &module->panelTheme : NULL));
			// mute for tracks 1 to 8
			addParam(createDynamicParamCentered<DynMuteButton>(mm2px(Vec(67.31  + 12.7 * i, 55.7)), module, AuxExpander::TRACK_AUXMUTE_PARAMS + i, module ? &module->panelTheme : NULL));
			
			
			// Labels for tracks 9 to 16
			addChild(trackAndGroupLabels[i + 8] = createWidgetCentered<TrackAndGroupLabel>(mm2px(Vec(67.31 + 12.7 * i, 65.08))));
			if (module) {
				trackAndGroupLabels[i + 8]->dispColor = &(module->colorAndCloak.cc4[dispColor]);
			}

			// aux A send for tracks 9 to 16
			addParam(createDynamicParamCentered<DynSmallKnobAuxA>(mm2px(Vec(67.31 + 12.7 * i, 74.5)), module, AuxExpander::TRACK_AUXSEND_PARAMS + (i + 8) * 4 + 0, module ? &module->panelTheme : NULL));			
			// aux B send for tracks 9 to 16
			addParam(createDynamicParamCentered<DynSmallKnobAuxB>(mm2px(Vec(67.31 + 12.7 * i, 85.35)), module, AuxExpander::TRACK_AUXSEND_PARAMS + (i + 8) * 4 + 1, module ? &module->panelTheme : NULL));
			// aux C send for tracks 9 to 16
			addParam(createDynamicParamCentered<DynSmallKnobAuxC>(mm2px(Vec(67.31 + 12.7 * i, 96.2)), module, AuxExpander::TRACK_AUXSEND_PARAMS + (i + 8) * 4 + 2, module ? &module->panelTheme : NULL));
			// aux D send for tracks 9 to 16
			addParam(createDynamicParamCentered<DynSmallKnobAuxD>(mm2px(Vec(67.31 + 12.7 * i, 107.05)), module, AuxExpander::TRACK_AUXSEND_PARAMS + (i + 8) * 4 + 3, module ? &module->panelTheme : NULL));
			// mute for tracks 1 to 8
			addParam(createDynamicParamCentered<DynMuteButton>(mm2px(Vec(67.31  + 12.7 * i, 116.1)), module, AuxExpander::TRACK_AUXMUTE_PARAMS + i + 8, module ? &module->panelTheme : NULL));
		}

		// Right side (individual groups)
		for (int i = 0; i < 2; i++) {
			// Labels for groups 1 to 2
			addChild(trackAndGroupLabels[i + 16] = createWidgetCentered<TrackAndGroupLabel>(mm2px(Vec(171.45 + 12.7 * i, 4.7))));
			if (module) {
				trackAndGroupLabels[i + 16]->dispColor = &(module->colorAndCloak.cc4[dispColor]);
			}

			// aux A send for groups 1 to 2
			addParam(createDynamicParamCentered<DynSmallKnobAuxA>(mm2px(Vec(171.45 + 12.7 * i, 14)), module, AuxExpander::GROUP_AUXSEND_PARAMS + i * 4 + 0, module ? &module->panelTheme : NULL));			
			// aux B send for groups 1 to 2
			addParam(createDynamicParamCentered<DynSmallKnobAuxB>(mm2px(Vec(171.45 + 12.7 * i, 24.85)), module, AuxExpander::GROUP_AUXSEND_PARAMS + i * 4 + 1, module ? &module->panelTheme : NULL));
			// aux C send for groups 1 to 2
			addParam(createDynamicParamCentered<DynSmallKnobAuxC>(mm2px(Vec(171.45 + 12.7 * i, 35.7)), module, AuxExpander::GROUP_AUXSEND_PARAMS + i * 4 + 2, module ? &module->panelTheme : NULL));
			// aux D send for groups 1 to 2
			addParam(createDynamicParamCentered<DynSmallKnobAuxD>(mm2px(Vec(171.45 + 12.7 * i, 46.55)), module, AuxExpander::GROUP_AUXSEND_PARAMS + i * 4 + 3, module ? &module->panelTheme : NULL));
			// mute for groups 1 to 2
			addParam(createDynamicParamCentered<DynMuteButton>(mm2px(Vec(171.45  + 12.7 * i, 55.7)), module, AuxExpander::GROUP_AUXMUTE_PARAMS + i, module ? &module->panelTheme : NULL));
			
			
			// Labels for groups 3 to 4
			addChild(trackAndGroupLabels[i + 18] = createWidgetCentered<TrackAndGroupLabel>(mm2px(Vec(171.45 + 12.7 * i, 65.08))));
			if (module) {
				trackAndGroupLabels[i + 18]->dispColor = &(module->colorAndCloak.cc4[dispColor]);
			}

			// aux A send for groups 3 to 4
			addParam(createDynamicParamCentered<DynSmallKnobAuxA>(mm2px(Vec(171.45 + 12.7 * i, 74.5)), module, AuxExpander::GROUP_AUXSEND_PARAMS + (i + 2) * 4 + 0, module ? &module->panelTheme : NULL));			
			// aux B send for groups 3 to 4
			addParam(createDynamicParamCentered<DynSmallKnobAuxB>(mm2px(Vec(171.45 + 12.7 * i, 85.35)), module, AuxExpander::GROUP_AUXSEND_PARAMS + (i + 2) * 4 + 1, module ? &module->panelTheme : NULL));
			// aux C send for groups 3 to 4
			addParam(createDynamicParamCentered<DynSmallKnobAuxC>(mm2px(Vec(171.45 + 12.7 * i, 96.2)), module, AuxExpander::GROUP_AUXSEND_PARAMS + (i + 2) * 4 + 2, module ? &module->panelTheme : NULL));
			// aux D send for groups 3 to 4
			addParam(createDynamicParamCentered<DynSmallKnobAuxD>(mm2px(Vec(171.45 + 12.7 * i, 107.05)), module, AuxExpander::GROUP_AUXSEND_PARAMS + (i + 2) * 4 + 3, module ? &module->panelTheme : NULL));
			// mute for groups 3 to 4
			addParam(createDynamicParamCentered<DynMuteButton>(mm2px(Vec(171.45  + 12.7 * i, 116.1)), module, AuxExpander::GROUP_AUXMUTE_PARAMS + i + 2, module ? &module->panelTheme : NULL));
		}
		
		// CV inputs A-D
		addInput(createDynamicPortCentered<DynPortGold>(mm2px(Vec(204.62, 13.8)), true, module, AuxExpander::POLY_AUX_AD_CV_INPUTS + 0, module ? &module->panelTheme : NULL));			
		addInput(createDynamicPortCentered<DynPortGold>(mm2px(Vec(204.62, 13.8 + 10.85 * 1)), true, module, AuxExpander::POLY_AUX_AD_CV_INPUTS + 1, module ? &module->panelTheme : NULL));			
		addInput(createDynamicPortCentered<DynPortGold>(mm2px(Vec(204.62, 13.8 + 10.85 * 2)), true, module, AuxExpander::POLY_AUX_AD_CV_INPUTS + 2, module ? &module->panelTheme : NULL));			
		addInput(createDynamicPortCentered<DynPortGold>(mm2px(Vec(204.62, 13.8 + 10.85 * 3)), true, module, AuxExpander::POLY_AUX_AD_CV_INPUTS + 3, module ? &module->panelTheme : NULL));	
		
		// CV input M
		addInput(createDynamicPortCentered<DynPortGold>(mm2px(Vec(204.62, 13.8 + 10.85 * 4)), true, module, AuxExpander::POLY_AUX_M_CV_INPUT, module ? &module->panelTheme : NULL));	
		
		// CV input grp A-D
		addInput(createDynamicPortCentered<DynPortGold>(mm2px(Vec(204.62, 13.8 + 10.85 * 5)), true, module, AuxExpander::POLY_GRPS_AD_CV_INPUT, module ? &module->panelTheme : NULL));	
		
		// CV input M grp
		addInput(createDynamicPortCentered<DynPortGold>(mm2px(Vec(204.62, 13.8 + 10.85 * 6)), true, module, AuxExpander::POLY_GRPS_M_CV_INPUT, module ? &module->panelTheme : NULL));	
		
		// CV input extra
		addInput(createDynamicPortCentered<DynPortGold>(mm2px(Vec(204.62, 13.8 + 10.85 * 7)), true, module, AuxExpander::POLY_BUS_CV_INPUT, module ? &module->panelTheme : NULL));	
	
	}
	
	
	json_t* toJson() override {
		json_t* rootJ = ModuleWidget::toJson();

		// aux0 label
		json_object_set_new(rootJ, "aux0label", json_string(auxDisplays[0]->text.c_str()));
		// aux1 label
		json_object_set_new(rootJ, "aux1label", json_string(auxDisplays[1]->text.c_str()));
		// aux2 label
		json_object_set_new(rootJ, "aux2label", json_string(auxDisplays[2]->text.c_str()));
		// aux3 label
		json_object_set_new(rootJ, "aux3label", json_string(auxDisplays[3]->text.c_str()));

		return rootJ;
	}

	void fromJson(json_t* rootJ) override {
		ModuleWidget::fromJson(rootJ);

		// aux0 label
		json_t* aux0J = json_object_get(rootJ, "aux0label");
		if (aux0J)
			auxDisplays[0]->text = json_string_value(aux0J);
		
		// aux1 label
		json_t* aux1J = json_object_get(rootJ, "aux1label");
		if (aux1J)
			auxDisplays[1]->text = json_string_value(aux1J);

		// aux2 label
		json_t* aux2J = json_object_get(rootJ, "aux2label");
		if (aux2J)
			auxDisplays[2]->text = json_string_value(aux2J);
		
		// aux3 label
		json_t* aux3J = json_object_get(rootJ, "aux3label");
		if (aux3J)
			auxDisplays[3]->text = json_string_value(aux3J);
	}



	void step() override {
		if (module) {
			AuxExpander* moduleA = (AuxExpander*)module;
			
			// Labels (pull from module)
			if (moduleA->resetAuxLabelRequest != 0) {
				// aux labels
				for (int aux = 0; aux < 4; aux++) {
					auxDisplays[aux]->text = getInitAuxLabel(aux);
				}
				
				moduleA->resetAuxLabelRequest = 0;
			}
			if (moduleA->updateTrackLabelRequest != 0) {// pull request from module
				// track and group labels
				for (int trk = 0; trk < 20; trk++) {
					trackAndGroupLabels[trk]->text = std::string(&(moduleA->trackLabels[trk * 4]), 4);
				}
				moduleA->updateTrackLabelRequest = 0;// all done pulling
			}
			
			// Borders			
			if ( moduleA->motherPresent != oldMotherPresent ) {
				oldMotherPresent = moduleA->motherPresent;
				if (oldMotherPresent) {
					panelBorder->box.pos.x = -3;
					panelBorder->box.size.x = box.size.x + 3;
				}
				else {
					panelBorder->box.pos.x = 0;
					panelBorder->box.size.x = box.size.x;
				}
				((SvgPanel*)panel)->dirty = true;// weird zoom bug: if the if/else above is commented, zoom bug when this executes
			}
		}
		Widget::step();
	}
	
};

Model *modelAuxExpander = createModel<AuxExpander, AuxExpanderWidget>("AuxExpander");
