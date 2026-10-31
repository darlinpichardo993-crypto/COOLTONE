#pragma once
#include <JuceHeader.h>
#include "ScaleEngine.h"
#include <array>
#include <atomic>
#include <vector>
class CoolToneAudioProcessor final : public juce::AudioProcessor
{
public:
 CoolToneAudioProcessor();
 void prepareToPlay(double,int) override;
 void releaseResources() override;
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override{return true;}
 const juce::String getName() const override{return "COOLTONE";}
 bool acceptsMidi() const override{return false;} bool producesMidi() const override{return false;} bool isMidiEffect() const override{return false;}
 double getTailLengthSeconds() const override{return 0.06;}
 int getNumPrograms() override{return 1;} int getCurrentProgram() override{return 0;} void setCurrentProgram(int) override{}
 const juce::String getProgramName(int) override{return {};} void changeProgramName(int,const juce::String&) override{}
 void getStateInformation(juce::MemoryBlock&) override; void setStateInformation(const void*,int) override;
 juce::AudioProcessorValueTreeState state;
 float pitch() const{return currentPitch.load();}
 float history(int age) const;
private:
 static juce::AudioProcessorValueTreeState::ParameterLayout makeParams();
 void analyse() noexcept;
 std::array<float,2048> ring{}; std::array<std::atomic<float>,256> trace{};
 std::atomic<float> currentPitch{-1}; std::atomic<int> tracePos{0};
 std::vector<std::vector<float>> delay;
 double sampleRate=44100,phase=0,vibPhase=0; int write=0,delayWrite=0,analysisCount=0,stable=0;
 float ratio=1,targetShift=0; cooltone::ScaleEngine scales;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CoolToneAudioProcessor)
};
