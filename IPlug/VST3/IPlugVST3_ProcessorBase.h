
/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#pragma once

#include "public.sdk/source/vst/vstbus.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstevents.h"

#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"
#include "IPlugVST3_Defs.h"

// Custom bus type function (in global namespace)
#ifdef CUSTOM_BUSTYPE_FUNC
extern uint64_t GetAPIBusTypeForChannelIOConfig(int configIdx, iplug::ERoute dir, int busIdx, const iplug::IOConfig* pConfig);
#endif

BEGIN_IPLUG_NAMESPACE

// Default bus type function (in iplug namespace)
#ifndef CUSTOM_BUSTYPE_FUNC
uint64_t GetAPIBusTypeForChannelIOConfig(int configIdx, ERoute dir, int busIdx, const IOConfig* pConfig);
#endif

/** Shared VST3 processor code */
class IPlugVST3ProcessorBase : public IPlugProcessor
{
public:
  IPlugVST3ProcessorBase(Config c, IPlugAPIBase& plug);
  
  template <class T>
  void Initialize(T* pPlug)
  {
    using namespace Steinberg::Vst;

    Steinberg::Vst::String128 tmpStringBuf;
    
//  for(auto configIdx = 0; configIdx < NIOConfigs(); configIdx++)
//  {
    int configIdx = NIOConfigs() - 1;
    
    const IOConfig* pConfig = GetIOConfig(configIdx);
    
    assert(pConfig);
    for (auto busIdx = 0; busIdx < pConfig->NBuses(ERoute::kInput); busIdx++)
    {
      uint64_t busType = GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kInput, busIdx, pConfig);
      
      int flags = 0; //busIdx == 0 ? flags = BusInfo::BusFlags::kDefaultActive : flags = 0;
      Steinberg::UString(tmpStringBuf, 128).fromAscii(pConfig->GetBusInfo(ERoute::kInput, busIdx)->GetLabel(), 128);
      pPlug->addAudioInput(tmpStringBuf, busType, (BusTypes) busIdx > 0, flags);
    }
    
    for (auto busIdx = 0; busIdx < pConfig->NBuses(ERoute::kOutput); busIdx++)
    {
      uint64_t busType = GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kOutput, busIdx, pConfig);
      
      int flags = 0; //busIdx == 0 ? flags = BusInfo::BusFlags::kDefaultActive : flags = 0;
      Steinberg::UString(tmpStringBuf, 128).fromAscii(pConfig->GetBusInfo(ERoute::kOutput, busIdx)->GetLabel(), 128);
      pPlug->addAudioOutput(tmpStringBuf, busType, (BusTypes) busIdx > 0, flags);
    }
//  }

    if (IsMidiEffect() && pConfig->NBuses(ERoute::kOutput) == 0)
    {
      int flags = 0;
      Steinberg::UString(tmpStringBuf, 128).fromAscii("Dummy Output", 128);
      pPlug->addAudioOutput(tmpStringBuf, Steinberg::Vst::SpeakerArr::kEmpty, Steinberg::Vst::BusTypes::kMain, flags);
    }
    
    if (DoesMIDIIn())
      pPlug->addEventInput(STR16("MIDI Input"), VST3_NUM_MIDI_IN_CHANS);
    
    if (DoesMIDIOut())
      pPlug->addEventOutput(STR16("MIDI Output"), VST3_NUM_MIDI_OUT_CHANS);
  }
  
  // MIDI Processing
  void ProcessMidiIn(Steinberg::Vst::IEventList* pEventList, IPlugQueue<IMidiMsg>& editorQueue, IPlugQueue<IMidiMsg>& processorQueue);
  void ProcessMidiOut(IPlugQueue<SysExData>& sysExQueue, SysExData& sysExBuf, Steinberg::Vst::IEventList* pOutputEvents, Steinberg::int32 numSamples);
  
  // Audio Processing Setup
  template <class T>
  bool SetBusArrangements(T* pPlug, Steinberg::Vst::SpeakerArrangement* pInputBusArrangements, Steinberg::int32 numInBuses, Steinberg::Vst::SpeakerArrangement* pOutputBusArrangements, Steinberg::int32 numOutBuses)
  {
    using namespace Steinberg::Vst;

    // disconnect all io pins, they will be reconnected in process
    SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), false);
    SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), false);
        
    int maxNInBuses = MaxNBuses(ERoute::kInput);
    int maxNOutBuses = MaxNBuses(ERoute::kOutput);
    
    if(numInBuses > maxNInBuses || numOutBuses > maxNOutBuses)
      return false;
    
    for(auto inBusIdx = 0; inBusIdx < maxNInBuses; inBusIdx++)
    {
      AudioBus* pBus = pPlug->getAudioInput(inBusIdx);
      int NInputsRequired = SpeakerArr::getChannelCount(pInputBusArrangements[inBusIdx]);

      // if existing input bus has a different number of channels to the input bus being connected
      if (pBus && SpeakerArr::getChannelCount(pBus->getArrangement()) != NInputsRequired)
      {
        pPlug->removeAudioInputBus(pBus);
        int flags = inBusIdx == 0 ? BusInfo::BusFlags::kDefaultActive : 0;
        SpeakerArrangement arr = GetAPIBusTypeForChannelIOConfig(0 /*TODO: fix*/, ERoute::kInput, inBusIdx, GetIOConfig(0 /*TODO: fix*/));
        pPlug->addAudioInput(USTRING("Input"), arr, (BusTypes) inBusIdx > 0, flags);
      }
    }
    
    for(auto outBusIdx = 0; outBusIdx < maxNOutBuses; outBusIdx++)
    {
      AudioBus* pBus = pPlug->getAudioOutput(outBusIdx);
      int NOutputsRequired = SpeakerArr::getChannelCount(pOutputBusArrangements[outBusIdx]);

      // if existing output bus has a different number of channels to the input bus being connected
      if (pBus && SpeakerArr::getChannelCount(pBus->getArrangement()) != NOutputsRequired)
      {
        pPlug->removeAudioOutputBus(pBus);
        int flags = outBusIdx == 0 ? BusInfo::BusFlags::kDefaultActive : 0;
        SpeakerArrangement arr = GetAPIBusTypeForChannelIOConfig(0 /*TODO: fix*/, ERoute::kOutput, outBusIdx, GetIOConfig(0 /*TODO: fix*/));
        pPlug->addAudioOutput(USTRING("Output"), arr, (BusTypes) outBusIdx > 0, flags);
      }
    }
    
    return true;
  }
  
  void AttachBuffers(ERoute direction, int idx, int n, Steinberg::Vst::AudioBusBuffers& pBus, int nFrames, Steinberg::int32 sampleSize);
  bool SetupProcessing(const Steinberg::Vst::ProcessSetup& setup, Steinberg::Vst::ProcessSetup& storedSetup);
  bool CanProcessSampleSize(Steinberg::int32 symbolicSampleSize);
  bool SetProcessing(bool state);
  
  // Audio Processing
  void PrepareProcessContext(Steinberg::Vst::ProcessData& data, Steinberg::Vst::ProcessSetup& setup);
  void ProcessParameterChanges(Steinberg::Vst::ProcessData& data);
  void ProcessAudio(Steinberg::Vst::ProcessData& data, Steinberg::Vst::ProcessSetup& setup, const Steinberg::Vst::BusList& ins, const Steinberg::Vst::BusList& outs);
  void Process(Steinberg::Vst::ProcessData& data, Steinberg::Vst::ProcessSetup& setup, const Steinberg::Vst::BusList& ins, const Steinberg::Vst::BusList& outs, IPlugQueue<IMidiMsg>& fromEditor, IPlugQueue<IMidiMsg>& fromProcessor, IPlugQueue<SysExData>& sysExFromEditor, SysExData& sysExBuf);
  
  // IPlugProcessor overrides
  bool SendMidiMsg(const IMidiMsg& msg) override;

private:
  IPlugAPIBase& mPlug;
  Steinberg::Vst::ProcessContext mProcessContext;
  IMidiQueue mMidiOutputQueue;
  bool mSidechainActive = false;
};

END_IPLUG_NAMESPACE
