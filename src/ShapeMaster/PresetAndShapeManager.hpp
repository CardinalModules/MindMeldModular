//***********************************************************************************************
//Mind Meld Modular: Modules for VCV Rack by Steve Baker and Marc Boulé
//
//Based on code from the Fundamental plugin by Andrew Belt 
//See ./LICENSE.md for all licenses
//***********************************************************************************************


#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#else
#include <thread>
#include <condition_variable>
#endif
#include "osdialog.h"
#include "Channel.hpp"


bool loadPresetOrShape(const std::string path, Channel* dest, bool isPreset, bool* unsupportedSync, bool withHistory);
void savePresetOrShape(const std::string path, Channel* dest, bool isPreset, Channel* channelDirtyCache);


// ----------------------------------------------------------------------------
// Preset and Shape Manager
// ----------------------------------------------------------------------------

enum WorkerState {WS_NONE, WS_STAGED, WS_TODO};// staged is not managed in here, it is a mem for sync locked scheduled shape change only
enum WorkType {WT_PREV_PRESET, WT_NEXT_PRESET, WT_PREV_SHAPE, WT_NEXT_SHAPE, WT_REVERSE, WT_INVERT, WT_RANDOM};


class PresetAndShapeManager {
	// general
	std::vector<std::string> factoryPresetVector;
	std::vector<std::string> factoryShapeVector;
	Channel* channels;
	Channel* channelDirtyCacheSrc;
	
	// worker
	int workType[8] = {};// this value is not used
	bool withHistory[8];
	int8_t requestWork[8] = {};
#ifdef __EMSCRIPTEN__
	long workerId;
	static void worker_cb(void* userData);
#else
	std::condition_variable cv;// https://thispointer.com//c11-multithreading-part-7-condition-variables-explained/
	std::mutex mtx;
	std::thread worker;// http://www.cplusplus.com/reference/thread/thread/thread/
	bool requestStop = false;
	Context* context;
#endif
		
	// other
	PackedBytes4* miscSettings3;
	
	
	public:


	PresetAndShapeManager();
	
	
	~PresetAndShapeManager() {
#ifdef __EMSCRIPTEN__
		emscripten_clear_interval(workerId);
#else
		std::unique_lock<std::mutex> lk(mtx);
		requestStop = true;
		lk.unlock();
		cv.notify_one();
		worker.join();
#endif
	}
	

	void construct(Channel* channels, Channel* _channelDirtyCacheSrc, PackedBytes4* _miscSettings3);
	

	void executeIfStaged(int c) {
		if (requestWork[c] == WS_STAGED) {
			requestWork[c] = WS_TODO;
#ifndef __EMSCRIPTEN__
			cv.notify_one();
#endif
		}
	}
	void executeAllIfStaged() {
		for (int c = 0; c < 8; c++) {
			executeIfStaged(c);
		}
	}
	
	
	void cleanWorkload(int c) {
		requestWork[c] = WS_NONE;
	}
	void clearAllWorkloads() {
		for (int c = 0; c < 8; c++) {
			requestWork[c] = WS_NONE;
		}
	}
	
	
	bool isDeferred(int c, int arrow) {
		if (requestWork[c] != WS_STAGED) return false;
		return arrow == workType[c];
	}
	
	
	void executeOrStageWorkload(int c, int _workType, bool _withHistory, bool stage);

	void file_worker();

	void createPresetOrShapeMenu(Channel* channel, bool isPreset);
};// PresetAndShapeManager



