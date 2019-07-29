//***********************************************************************************************
//Mixer module for VCV Rack by Steve Baker and Marc Boulé 
//
//Based on code from the Fundamental plugin by Andrew Belt 
//See ./LICENSE.txt for all licenses
//***********************************************************************************************

#ifndef MMM_MIXERWIDGETS_HPP
#define MMM_MIXERWIDGETS_HPP


#include "Mixer.hpp"



// VU meters
// --------------------

struct VuMeterBase : OpaqueWidget {
	static constexpr float epsilon = 0.001f;// don't show VUs below 1mV
	const NVGcolor PEAK_GREEN = nvgRGB(45, 133, 52);// 122, 201, 67
	const NVGcolor RMS_GREEN = nvgRGB(30, 254, 75);
	
	// instantiator must setup:
	VuMeterAll *srcLevels;// from 0 to 10 V, with 10 V = 0dB (since -10 to 10 is the max)
	
	// derived class must setup:
	float gapX;// in px
	float barX;// in px
	float barY;// in px
	// box.size // inherited, no need to declare
	float faderMaxLinearGain;
	float faderScalingExponent;
	float zeroDbVoltage;
	
	
	void draw(const DrawArgs &args) override {
		// TODO: use time() that is used in Foundry to keep track of max in here, not in mixer process, to display peakhold, and reset every n seconds.
		
		nvgBeginPath(args.vg);

		// peak values
		drawVu(args, srcLevels[0].getPeak(), 0);
		drawVu(args, srcLevels[1].getPeak(), barX + gapX);
		nvgFillColor(args.vg, PEAK_GREEN);
		nvgFill(args.vg);

		nvgBeginPath(args.vg);
		
		// rms values
		drawVu(args, srcLevels[0].getRms(), 0);
		drawVu(args, srcLevels[1].getRms(), barX + gapX);
		nvgFillColor(args.vg, RMS_GREEN);
		nvgFill(args.vg);
		
		Widget::draw(args);
	}
	
	void drawVu(const DrawArgs &args, float peakValue, float posX) {
		if (peakValue >= epsilon) {
			float vuHeight = peakValue / (faderMaxLinearGain * zeroDbVoltage);
			vuHeight = std::pow(vuHeight, 1.0f / faderScalingExponent);
			vuHeight = std::min(vuHeight, 1.0f);// normalized is now clamped
			vuHeight *= barY;
			nvgRect(args.vg, posX, barY - vuHeight, barX, vuHeight);
		}
	}
};

// 2.8mm x 42mm
struct VuMeterTrack : VuMeterBase {//
	VuMeterTrack() {
		gapX = mm2px(0.4);
		barX = mm2px(1.2);
		barY = mm2px(42.0);
		box.size = Vec(barX * 2 + gapX, barY);
		faderMaxLinearGain = MixerTrack::trackFaderMaxLinearGain;
		faderScalingExponent = MixerTrack::trackFaderScalingExponent;
		zeroDbVoltage = 5.0f;
	}
};

// 3.8mm x 60mm
struct VuMeterMaster : VuMeterBase {
	VuMeterMaster() {
		gapX = mm2px(0.6);
		barX = mm2px(1.6);
		barY = mm2px(60.0);
		box.size = Vec(barX * 2 + gapX, barY);
		faderMaxLinearGain = GlobalInfo::masterFaderMaxLinearGain;
		faderScalingExponent = GlobalInfo::masterFaderScalingExponent;
		zeroDbVoltage = 10.0f;
	}
};



// Track context menu
// --------------------

// Gain adjust

struct GainAdjustQuantity : Quantity {
	MixerTrack *srcTrack = NULL;
	float gainInDB = 0.0f;
	  
	GainAdjustQuantity(MixerTrack *_srcTrack) {
		srcTrack = _srcTrack;
	}
	void setValue(float value) override {
		gainInDB = math::clamp(value, getMinValue(), getMaxValue());
		srcTrack->gainAdjust = std::pow(10.0f, gainInDB / 20.0f);
	}
	float getValue() override {
		gainInDB = 20.0f * std::log10(srcTrack->gainAdjust);
		return gainInDB;
	}
	float getMinValue() override {return -20.0f;}
	float getMaxValue() override {return 20.0f;}
	float getDefaultValue() override {return 0.0f;}
	float getDisplayValue() override {return getValue();}
	std::string getDisplayValueString() override {
		float valGain = getDisplayValue();
		valGain =  std::round(valGain * 100.0f);
		return string::f("%g", math::normalizeZero(valGain / 100.0f));
	}
	void setDisplayValue(float displayValue) override {setValue(displayValue);}
	std::string getLabel() override {return "Gain adjust";}
	std::string getUnit() override {return " dB";}
};

struct GainAdjustSlider : ui::Slider {
	GainAdjustSlider(MixerTrack *srcTrack) {
		quantity = new GainAdjustQuantity(srcTrack);
	}
	~GainAdjustSlider() {
		delete quantity;
	}
};


// HPF filter cutoff

struct HPFCutoffQuantity : Quantity {
	MixerTrack *srcTrack = NULL;
	
	HPFCutoffQuantity(MixerTrack *_srcTrack) {
		srcTrack = _srcTrack;
	}
	void setValue(float value) override {
		srcTrack->setHPFCutoffFreq(math::clamp(value, getMinValue(), getMaxValue()));
	}
	float getValue() override {
		return srcTrack->getHPFCutoffFreq();
	}
	float getMinValue() override {return 13.0f;}
	float getMaxValue() override {return 350.0f;}
	float getDefaultValue() override {return 13.0f;}
	float getDisplayValue() override {return getValue();}
	std::string getDisplayValueString() override {
		float valCut = getDisplayValue();
		if (valCut >= MixerTrack::minHPFCutoffFreq) {
			return string::f("%i", (int)(math::normalizeZero(valCut) + 0.5f));
		}
		else {
			return "OFF";
		}
	}
	void setDisplayValue(float displayValue) override {setValue(displayValue);}
	std::string getLabel() override {return "HPF Cutoff";}
	std::string getUnit() override {
		if (getDisplayValue() >= MixerTrack::minHPFCutoffFreq) {
			return " Hz";
		}
		else {
			return "";
		}
	}
};

struct HPFCutoffSlider : ui::Slider {
	HPFCutoffSlider(MixerTrack *srcTrack) {
		quantity = new HPFCutoffQuantity(srcTrack);
	}
	~HPFCutoffSlider() {
		delete quantity;
	}
};



// LPF filter cutoff

struct LPFCutoffQuantity : Quantity {
	MixerTrack *srcTrack = NULL;
	
	LPFCutoffQuantity(MixerTrack *_srcTrack) {
		srcTrack = _srcTrack;
	}
	void setValue(float value) override {
		srcTrack->setLPFCutoffFreq(math::clamp(value, getMinValue(), getMaxValue()));
	}
	float getValue() override {
		return srcTrack->getLPFCutoffFreq();
	}
	float getMinValue() override {return 3000.0f;}
	float getMaxValue() override {return 21000.0f;}
	float getDefaultValue() override {return 20010.0f;}
	float getDisplayValue() override {return getValue();}
	std::string getDisplayValueString() override {
		float valCut = getDisplayValue();
		if (valCut <= MixerTrack::maxLPFCutoffFreq) {
			valCut =  std::round(valCut / 100.0f);
			return string::f("%g", math::normalizeZero(valCut / 10.0f));
		}
		else {
			return "OFF";
		}
	}
	void setDisplayValue(float displayValue) override {setValue(displayValue);}
	std::string getLabel() override {return "LPF Cutoff";}
	std::string getUnit() override {
		if (getDisplayValue() <= MixerTrack::maxLPFCutoffFreq) {
			return " kHz";
		}
		else {
			return "";
		}
	}
};

struct LPFCutoffSlider : ui::Slider {
	LPFCutoffSlider(MixerTrack *srcTrack) {
		quantity = new LPFCutoffQuantity(srcTrack);
	}
	~LPFCutoffSlider() {
		delete quantity;
	}
};




// Track display editable label with menu
// --------------------


struct TrackDisplay : LedDisplayTextField {
	MixerTrack *srcTrack = NULL;

	TrackDisplay() {
		box.size = Vec(38, 16);
		textOffset = Vec(2.6f, -2.2f);
		text = "-00-";
	};
	
	// don't want background so implement adapted version here
	void draw(const DrawArgs &args) override {
		// override and do not call LedDisplayTextField.draw() since draw ourselves
		if (cursor > 4) {
			text.resize(4);
			cursor = 4;
			selection = 4;
		}
		
		// the code below is from LedDisplayTextField.draw()
		nvgScissor(args.vg, RECT_ARGS(args.clipBox));
		if (font->handle >= 0) {
			bndSetFont(font->handle);

			NVGcolor highlightColor = color;
			highlightColor.a = 0.5;
			int begin = std::min(cursor, selection);
			int end = (this == APP->event->selectedWidget) ? std::max(cursor, selection) : -1;
			bndIconLabelCaret(args.vg, textOffset.x, textOffset.y,
				box.size.x - 2*textOffset.x, box.size.y - 2*textOffset.y,
				-1, color, 12, text.c_str(), highlightColor, begin, end);

			bndSetFont(APP->window->uiFont->handle);
		}
		nvgResetScissor(args.vg);
	}
	
	// don't want spaces since leading spaces are stripped by nanovg (which oui-blendish calls), so convert to dashes
	void onSelectText(const event::SelectText &e) override {
		if (cursor < 4) {
			//LedDisplayTextField::onSelectText(e); // copied below to morph spaces
			if (e.codepoint < 128) {
				char letter = (char) e.codepoint;
				if (letter == 0x20) {// space
					letter = 0x2D;// hyphen
				}
				std::string newText(1, letter);
				insertText(newText);
			}
			e.consume(this);	
			
			if (text.length() > 4) {
				text = text.substr(0, 4);
			}
		}
		else {
			e.consume(this);
		}
	}


	void onButton(const event::Button &e) override {
		if (e.button == GLFW_MOUSE_BUTTON_RIGHT && e.action == GLFW_PRESS) {
			ui::Menu *menu = createMenu();

			MenuLabel *lowcutLabel = new MenuLabel();
			lowcutLabel->text = "Track settings: ";
			menu->addChild(lowcutLabel);
			
			GainAdjustSlider *trackGainAdjustSlider = new GainAdjustSlider(srcTrack);
			trackGainAdjustSlider->box.size.x = 200.0f;
			menu->addChild(trackGainAdjustSlider);
			
			HPFCutoffSlider *trackHPFAdjustSlider = new HPFCutoffSlider(srcTrack);
			trackHPFAdjustSlider->box.size.x = 200.0f;
			menu->addChild(trackHPFAdjustSlider);
			
			LPFCutoffSlider *trackLPFAdjustSlider = new LPFCutoffSlider(srcTrack);
			trackLPFAdjustSlider->box.size.x = 200.0f;
			menu->addChild(trackLPFAdjustSlider);
			
			e.consume(this);
			return;
		}
		LedDisplayTextField::onButton(e);
	}
	void onChange(const event::Change &e) override {
		(*((uint32_t*)(srcTrack->trackName))) = 0x20202020;
		for (int i = 0; i < std::min(4, (int)text.length()); i++) {
			srcTrack->trackName[i] = text[i];
		}
		LedDisplayTextField::onChange(e);
	};
};


#endif
