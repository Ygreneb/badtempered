//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Examples
// Filename    : plugprocessor.cpp
// Created by  : Steinberg, 01/2018
// Description : HelloWorld Example for VST 3
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2020, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "../include/plugprocessor.h"
#include "../include/plugids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

namespace Benergy {
namespace BadTempered {

//-----------------------------------------------------------------------------
PlugProcessor::PlugProcessor ()
{
	// register its editor class
	setControllerClass (MyControllerUID);

	mParameterState.volume = 0.0;
	mParameterState.tuning = 0.0;
	mParameterState.rootNote = 0.0;
	mParameterState.bypass = false;

	mParameterState.attack = 0.0;
	mParameterState.decay = 0.0;
	mParameterState.sustain = 0.0;
	mParameterState.release = 0.0;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API PlugProcessor::initialize (FUnknown* context)
{
	//---always initialize the parent-------
	tresult result = AudioEffect::initialize (context);
	if (result != kResultTrue)
		return kResultFalse;

	//---create Audio In/Out buses------
	// we want a stereo Input and a Stereo Output
	addEventInput(STR16("EventInput"), 16, Vst::BusTypes::kMain);
	addEventInput(STR16("BassEventInput"), 16, Vst::BusTypes::kAux);
	addAudioOutput (STR16 ("AudioOutput"), Vst::SpeakerArr::kStereo);

	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API PlugProcessor::setBusArrangements (Vst::SpeakerArrangement* inputs,
                                                            int32 numIns,
                                                            Vst::SpeakerArrangement* outputs,
                                                            int32 numOuts)
{
	// we only support one in and output bus and these buses must have the same number of channels
	//if (numIns == 1 && numOuts == 1 && inputs[0] == outputs[0])
	if (numOuts == 1 && outputs[0] == Vst::SpeakerArr::kStereo)
	{
		return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API PlugProcessor::setupProcessing (Vst::ProcessSetup& setup)
{
	// here you get, with setup, information about:
	// sampleRate, processMode, maximum number of samples per audio block
	mProcessSetup = setup;
	return AudioEffect::setupProcessing (setup);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API PlugProcessor::setActive (TBool state)
{
	if (state) // Initialize
	{
		// Allocate Memory Here
		// Ex: algo.create ();
		if (!mVoiceProcessor)
		{
			mVoiceProcessor = new Vst::VoiceProcessorImplementation<float, Voice<float>, 2, MAX_VOICES, GlobalParameterState>(mProcessSetup.sampleRate, &mParameterState);
		}
	}
	else // Release
	{
		// Free Memory if still allocated
		// Ex: if(algo.isCreated ()) { algo.destroy (); }
		if (mVoiceProcessor != nullptr)
		{
			delete mVoiceProcessor;
		}
		mVoiceProcessor = nullptr;
	}
	return AudioEffect::setActive (state);
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API PlugProcessor::process (Vst::ProcessData& data)
{
	//--- Read inputs parameter changes-----------
	if (data.inputParameterChanges)
	{
		int32 numParamsChanged = data.inputParameterChanges->getParameterCount ();
		for (int32 index = 0; index < numParamsChanged; index++)
		{
			Vst::IParamValueQueue* paramQueue =
			    data.inputParameterChanges->getParameterData (index);
			if (paramQueue)
			{
				Vst::ParamValue value;
				int32 sampleOffset;
				int32 numPoints = paramQueue->getPointCount ();
				if (paramQueue->getPoint(numPoints - 1, sampleOffset, value) == kResultTrue)
				{
					switch (paramQueue->getParameterId())
					{
					case BadTemperedParams::kBypassId:
						mParameterState.bypass = (value > 0.5f);
						break;
					case BadTemperedParams::kVolumeId:
						mParameterState.volume = value;
						break;
					case BadTemperedParams::kTuningId:
						mParameterState.tuning = value;
						break;
					case BadTemperedParams::kRootNoteId:
						mParameterState.rootNote = value;
						break;
					case BadTemperedParams::kAttackId:
						mParameterState.attack = value;
						break;
					case BadTemperedParams::kDecayId:
						mParameterState.decay = value;
						break;
					case BadTemperedParams::kSustainId:
						mParameterState.sustain = value;
						break;
					case BadTemperedParams::kReleaseId:
						mParameterState.release = value;
						break;
					}
				}
			}
		}
	}

	//--- Process Audio---------------------
	//--- ----------------------------------
	if (data.numOutputs < 1 || data.numSamples < 1)
	{
		// nothing to do
		return kResultOk;
	}

	if (mVoiceProcessor != nullptr)
	{
		// Update tuning
		Vst::IEventList* inputEvents = data.inputEvents;
		int32 numEvents = inputEvents ? inputEvents->getEventCount() : 0;

		if (numEvents > 0)
		{
			Vst::Event e;
			for (int i = 0; i < numEvents; ++i)
			{
				inputEvents->getEvent(i, e);
				if (e.type == Vst::Event::kNoteOnEvent && e.busIndex == 1)
				{
					mParameterState.rootNote = e.noteOn.pitch;
				}
			}
		}

		// Main processing
		bool res = mVoiceProcessor->process(data);

		// Update root note param
		if (data.outputParameterChanges)
		{
			int32 index;
			auto paramQueue = data.outputParameterChanges->addParameterData(BadTemperedParams::kRootNoteId, index);
			if (paramQueue)
				paramQueue->addPoint(0, mParameterState.rootNote / 128.0, index);
		}

		return res;
	}

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API PlugProcessor::setState (IBStream* state)
{
	return mParameterState.setState(state);
}

//------------------------------------------------------------------------
tresult PLUGIN_API PlugProcessor::getState (IBStream* state)
{
	return mParameterState.getState(state);
}

//------------------------------------------------------------------------
} // namespace
} // namespace Benergy
