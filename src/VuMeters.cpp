//***********************************************************************************************
//Mixer module for VCV Rack by Steve Baker and Marc Boulé 
//
//Based on code from the Fundamental plugin by Andrew Belt 
//See ./LICENSE.md for all licenses
//***********************************************************************************************


#include "VuMeters.hpp"


// VuMeter signal processing code for peak/rms
// ----------------------------------------------------------------------------

// all in header file



// VuMeter displays
// ----------------------------------------------------------------------------

// VuMeterBase

void VuMeterBase::prepareYellowAndRedThresholds(float yellowMinDb, float redMinDb) {
	float maxLin = std::pow(faderMaxLinearGain, 1.0f / faderScalingExponent);
	float yellowLin = std::pow(std::pow(10.0f, yellowMinDb / 20.0f), 1.0f / faderScalingExponent);
	yellowThreshold = barY * (yellowLin / maxLin);
	float redLin = std::pow(std::pow(10.0f, redMinDb / 20.0f), 1.0f / faderScalingExponent);
	redThreshold = barY * (redLin / maxLin);
}


void VuMeterBase::processPeakHold() {
	long newTime = time(0);
	if ( (newTime != oldTime) && ((newTime & 0x1) == 0) ) {
		oldTime = newTime;
		peakHold[0] = 0.0f;
		peakHold[1] = 0.0f;
	}		
	for (int i = 0; i < 2; i++) {
		if (srcLevels->getPeak(i) > peakHold[i]) {
			peakHold[i] = srcLevels->getPeak(i);
		}
	}
}


void VuMeterBase::draw(const DrawArgs &args) {
	processPeakHold();
	
	setColor();
	
	// PEAK
	drawVu(args, srcLevels->getPeak(0), 0, 0);
	drawVu(args, srcLevels->getPeak(1), barX + gapX, 0);

	// RMS
	drawVu(args, srcLevels->getRms(0), 0, 1);
	drawVu(args, srcLevels->getRms(1), barX + gapX, 1);
	
	// PEAK_HOLD
	drawPeakHold(args, peakHold[0], 0);
	drawPeakHold(args, peakHold[1], barX + gapX);
			
	Widget::draw(args);
}



// VuMeterTrack

void VuMeterTrack::drawVu(const DrawArgs &args, float vuValue, float posX, int colorIndex) {
	if (vuValue >= epsilon) {

		float vuHeight = vuValue / (faderMaxLinearGain * zeroDbVoltage);
		vuHeight = std::pow(vuHeight, 1.0f / faderScalingExponent);
		vuHeight = std::min(vuHeight, 1.0f);// normalized is now clamped
		vuHeight *= barY;

		bool ghostMuteOn = (srcMuteGhost != NULL && *srcMuteGhost == 0.0f);
		NVGcolor colTop = ghostMuteOn ? VU_GRAY_TOP[colorIndex] : VU_THEMES_TOP[colorTheme][colorIndex];
		NVGcolor colBot = ghostMuteOn ? VU_GRAY_BOT[colorIndex] : VU_THEMES_BOT[colorTheme][colorIndex];
		NVGpaint gradGreen = nvgLinearGradient(args.vg, 0, barY - redThreshold, 0, barY, colTop, colBot);
		
		if (vuHeight > redThreshold) {
			// Yellow-Red gradient
			NVGcolor colTopRed = ghostMuteOn ? VU_GRAY_TOP[colorIndex] : VU_RED[colorIndex];
			NVGcolor colTopYel = ghostMuteOn ? VU_GRAY_TOP[colorIndex] : VU_YELLOW[colorIndex];
			NVGpaint gradTop = nvgLinearGradient(args.vg, 0, 0, 0, barY - redThreshold - sepYtrack, colTopRed, colTopYel);
			nvgBeginPath(args.vg);
			nvgRect(args.vg, posX, barY - vuHeight - sepYtrack, barX, vuHeight - redThreshold);
			nvgFillPaint(args.vg, gradTop);
			nvgFill(args.vg);
			// Green
			nvgBeginPath(args.vg);
			nvgRect(args.vg, posX, barY - redThreshold, barX, redThreshold);
			nvgFillPaint(args.vg, gradGreen);
			nvgFill(args.vg);			
		}
		else {
			// Green
			nvgBeginPath(args.vg);
			nvgRect(args.vg, posX, barY - vuHeight, barX, vuHeight);
			nvgFillPaint(args.vg, gradGreen);
			nvgFill(args.vg);
		}

	}
}

void VuMeterTrack::drawPeakHold(const DrawArgs &args, float holdValue, float posX) {
	if (holdValue >= epsilon) {
		float vuHeight = holdValue / (faderMaxLinearGain * zeroDbVoltage);
		vuHeight = std::pow(vuHeight, 1.0f / faderScalingExponent);
		vuHeight = std::min(vuHeight, 1.0f);// normalized is now clamped
		vuHeight *= barY;
		
		bool ghostMuteOn = (srcMuteGhost != NULL && *srcMuteGhost == 0.0f);
		if (vuHeight > redThreshold) {
			// Yellow-Red gradient
			NVGcolor colTopRed = ghostMuteOn ? VU_GRAY_TOP[1] : VU_RED[1];
			NVGcolor colTopYel = ghostMuteOn ? VU_GRAY_TOP[1] : VU_YELLOW[1];
			NVGpaint gradTop = nvgLinearGradient(args.vg, 0, 0, 0, barY - redThreshold - sepYtrack, colTopRed, colTopYel);
			nvgBeginPath(args.vg);
			nvgRect(args.vg, posX, barY - vuHeight - sepYtrack - peakHoldThick, barX, peakHoldThick);
			nvgFillPaint(args.vg, gradTop);
			nvgFill(args.vg);	
		}
		else {
			// Green
			NVGcolor colTop = ghostMuteOn ? VU_GRAY_TOP[1] : VU_THEMES_TOP[colorTheme][1];
			NVGcolor colBot = ghostMuteOn ? VU_GRAY_BOT[1] : VU_THEMES_BOT[colorTheme][1];
			NVGpaint gradGreen = nvgLinearGradient(args.vg, 0, barY - redThreshold, 0, barY, colTop, colBot);
			nvgBeginPath(args.vg);
			nvgRect(args.vg, posX, barY - vuHeight, barX, peakHoldThick);
			nvgFillPaint(args.vg, gradGreen);
			nvgFill(args.vg);
		}
	}		
}



// VuMeterMaster

void VuMeterMaster::drawVu(const DrawArgs &args, float vuValue, float posX, int colorIndex) {
	if (posX == 0) { // draw the separator for master since depends on softclip on/off. draw only once per vu pair
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0 - 1, barY - redThreshold - sepYmaster, box.size.x + 2, sepYmaster);
		nvgFillColor(args.vg, nvgRGB(53, 53, 53));
		nvgFill(args.vg);
	}

	if (vuValue >= epsilon) {
		float vuHeight = vuValue / (faderMaxLinearGain * zeroDbVoltage);
		vuHeight = std::pow(vuHeight, 1.0f / faderScalingExponent);
		vuHeight = std::min(vuHeight, 1.0f);// normalized is now clamped
		vuHeight *= barY;
		
		bool ghostMuteOn = (srcMuteGhost != NULL && *srcMuteGhost == 0.0f);
		float peakHoldVal = (posX == 0 ? peakHold[0] : peakHold[1]);
		if (!ghostMuteOn && (vuHeight > redThreshold || peakHoldVal > hardRedVoltage)) {
			// Full red
			nvgBeginPath(args.vg);
			if (vuHeight > redThreshold) {
				nvgRect(args.vg, posX, barY - vuHeight - sepYmaster, barX, vuHeight - redThreshold);
				nvgRect(args.vg, posX, barY - redThreshold, barX, redThreshold);
			}
			else {
				nvgRect(args.vg, posX, barY - vuHeight, barX, vuHeight);
			}
			nvgFillColor(args.vg, VU_RED[colorIndex]);
			nvgFill(args.vg);
		}
		else {
			NVGcolor colTop = ghostMuteOn ? VU_GRAY_TOP[colorIndex] : VU_THEMES_TOP[colorTheme][colorIndex];
			NVGcolor colBot = ghostMuteOn ? VU_GRAY_BOT[colorIndex] : VU_THEMES_BOT[colorTheme][colorIndex];
			NVGpaint gradGreen = nvgLinearGradient(args.vg, 0, barY - yellowThreshold, 0, barY, colTop, colBot);
			if (vuHeight > yellowThreshold) {
				// Yellow-Orange gradient
				NVGcolor colTopOra = ghostMuteOn ? VU_GRAY_TOP[colorIndex] : VU_ORANGE[colorIndex];
				NVGcolor colTopYel = ghostMuteOn ? VU_GRAY_TOP[colorIndex] : VU_YELLOW[colorIndex];
				NVGpaint gradTop = nvgLinearGradient(args.vg, 0, barY - redThreshold, 0, barY - yellowThreshold, colTopOra, colTopYel);
				nvgBeginPath(args.vg);
				nvgRect(args.vg, posX, barY - vuHeight, barX, vuHeight - yellowThreshold);
				nvgFillPaint(args.vg, gradTop);
				nvgFill(args.vg);
				// Green
				nvgBeginPath(args.vg);
				nvgRect(args.vg, posX, barY - yellowThreshold, barX, yellowThreshold);
				nvgFillPaint(args.vg, gradGreen);
				nvgFill(args.vg);			
			}
			else {
				// Green
				nvgBeginPath(args.vg);
				nvgRect(args.vg, posX, barY - vuHeight, barX, vuHeight);
				nvgFillPaint(args.vg, gradGreen);
				nvgFill(args.vg);
			}
		}
	}
}	

void VuMeterMaster::drawPeakHold(const DrawArgs &args, float holdValue, float posX) {
	if (holdValue >= epsilon) {
		float vuHeight = holdValue / (faderMaxLinearGain * zeroDbVoltage);
		vuHeight = std::pow(vuHeight, 1.0f / faderScalingExponent);
		vuHeight = std::min(vuHeight, 1.0f);// normalized is now clamped
		vuHeight *= barY;
		
		bool ghostMuteOn = (srcMuteGhost != NULL && *srcMuteGhost == 0.0f);
		float peakHoldVal = (posX == 0 ? peakHold[0] : peakHold[1]);
		if (!ghostMuteOn && (vuHeight > redThreshold || peakHoldVal > hardRedVoltage)) {
			// Full red
			nvgBeginPath(args.vg);
			nvgRect(args.vg, posX, barY - vuHeight - sepYmaster - peakHoldThick, barX, peakHoldThick);
			nvgFillColor(args.vg, VU_RED[1]);
			nvgFill(args.vg);
		} 
		else if (vuHeight > yellowThreshold) {
			// Yellow-Orange gradient
			NVGcolor colTopOra = ghostMuteOn ? VU_GRAY_TOP[1] : VU_ORANGE[1];
			NVGcolor colTopYel = ghostMuteOn ? VU_GRAY_TOP[1] : VU_YELLOW[1];
			NVGpaint gradTop = nvgLinearGradient(args.vg, 0, barY - redThreshold, 0, barY - yellowThreshold, colTopOra, colTopYel);
			nvgBeginPath(args.vg);
			nvgRect(args.vg, posX, barY - vuHeight, barX, 1.0);
			nvgFillPaint(args.vg, gradTop);
			nvgFill(args.vg);	
		}
		else {
			// Green
			NVGcolor colTop = ghostMuteOn ? VU_GRAY_TOP[1] : VU_THEMES_TOP[colorTheme][1];
			NVGcolor colBot = ghostMuteOn ? VU_GRAY_BOT[1] : VU_THEMES_BOT[colorTheme][1];
			NVGpaint gradGreen = nvgLinearGradient(args.vg, 0, barY - yellowThreshold, 0, barY, colTop, colBot);
			nvgBeginPath(args.vg);
			nvgRect(args.vg, posX, barY - vuHeight, barX, 1.0);
			nvgFillPaint(args.vg, gradGreen);
			nvgFill(args.vg);
		}
	}
}
