//***********************************************************************************************
//Mixer module for VCV Rack by Steve Baker and Marc Boulé 
//
//Based on code from the Fundamental plugin by Andrew Belt 
//See ./LICENSE.md for all licenses
//***********************************************************************************************

#ifndef MMM_EQ_COMMON_HPP
#define MMM_EQ_COMMON_HPP


#include "MindMeldModular.hpp"
#include "dsp/QuattroBiQuad.hpp"
#include "dsp/VuMeterAll.hpp"


static const bool DEFAULT_active = false;
static const bool DEFAULT_freqActive = false;
static const float DEFAULT_freq = 0.5f;
static const float DEFAULT_gain = 0.5f;
static const float DEFAULT_q = 0.5f;
static const bool DEFAULT_lowBP = false;
static const bool DEFAULT_highBP = false;


class TrackEq {
	bool active;
	bool freqActive[4];// frequency part is active, one for LF, LMF, HMF, HF
	float freq[4];
	float gain[4];
	float q[4];
	bool lowBP;// LF is band pass/cut when true (false is LPF)
	bool highBP;// HF is band pass/cut when true (false is HPF)
	// don't forget to also update to/from Json when adding to this struct
	
	QuattroBiQuad eqs[2];
	
	public:
	
	void init() {
		setActive(DEFAULT_active);
		for (int i = 0; i < 4; i++) {
			setFreqActive(i, DEFAULT_freqActive);
			setFreq(i, DEFAULT_freq);
			setGain(i, DEFAULT_gain);
			setQ(i, DEFAULT_q);
		}
		setLowBP(DEFAULT_lowBP);
		setHighBP(DEFAULT_highBP);
		eqs[0].reset();
		eqs[1].reset();
		eqs[0].setParameters(dsp::BiquadFilter::LOWSHELF, 0, 0.1f, 1.0f, 0.707f);
		eqs[1].setParameters(dsp::BiquadFilter::LOWSHELF, 0, 0.1f, 1.0f, 0.707f);
		eqs[0].setParameters(dsp::BiquadFilter::PEAK, 1, 0.1f, 1.0f, 0.707f);
		eqs[1].setParameters(dsp::BiquadFilter::PEAK, 1, 0.1f, 1.0f, 0.707f);
		eqs[0].setParameters(dsp::BiquadFilter::PEAK, 2, 0.1f, 1.0f, 0.707f);
		eqs[1].setParameters(dsp::BiquadFilter::PEAK, 2, 0.1f, 1.0f, 0.707f);
		eqs[0].setParameters(dsp::BiquadFilter::HIGHSHELF, 3, 0.01f, 0.1f, 0.707f);
		eqs[1].setParameters(dsp::BiquadFilter::HIGHSHELF, 3, 0.01f, 0.1f, 0.707f);
	}
	
	bool getActive() {return active;}
	bool getFreqActive(int eqNum) {return freqActive[eqNum];}
	float getFreq(int eqNum) {return freq[eqNum];}
	float getGain(int eqNum) {return gain[eqNum];}
	float getQ(int eqNum) {return q[eqNum];}
	float getLowBP() {return lowBP;}
	float getHighBP() {return highBP;}
	
	void setActive(bool _active) {active = _active;}
	void setFreqActive(int eqNum, bool _freqActive) {freqActive[eqNum] = _freqActive;}
	void setFreq(int eqNum, float _freq) {freq[eqNum] = _freq;}
	void setGain(int eqNum, float _gain) {gain[eqNum] = _gain;}
	void setQ(int eqNum, float _q) {q[eqNum] = _q;}
	void setLowBP(bool _lowBP) {lowBP = _lowBP;}
	void setHighBP(bool _highBP) {highBP = _highBP;}
	
	float processL(float inL) {return eqs[0].process(inL);}
	float processR(float inR) {return eqs[1].process(inR);}
};


#endif