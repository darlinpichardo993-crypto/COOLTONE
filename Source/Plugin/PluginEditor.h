#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PitchDetection.h"
#include "NoteSegmenter.h"
#include <vector>
#include <functional>
#include <utility>

class ExternalFileDragButton final : public juce::TextButton
{
public:
    explicit ExternalFileDragButton (const juce::String& text) : juce::TextButton(text) {}
    std::function<juce::String()> getFileToDrag;
    void mouseDown (const juce::MouseEvent& e) override { dragStarted=false; juce::TextButton::mouseDown(e); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        juce::TextButton::mouseDrag(e);
        if (dragStarted || !getFileToDrag) return;
        const auto path=getFileToDrag();
        if (path.isNotEmpty() && juce::File(path).existsAsFile())
            dragStarted=juce::DragAndDropContainer::performExternalDragDropOfFiles({path},false,this);
    }
private:
    bool dragStarted=false;
};

class CoolToneAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer,
                                           private juce::FileDragAndDropTarget
{
public:
    explicit CoolToneAudioProcessorEditor (CoolToneAudioProcessor&);
    ~CoolToneAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    void add (juce::Slider&, juce::Label&, const juce::String&);
    void openAudio();
    bool loadAudioFile (const juce::File&);
    void analyseAudio();
    bool exportTunedAudio();
    void drawNoteEditor (juce::Graphics&, juce::Rectangle<float>);
    int noteAtPosition (juce::Point<float>) const;

    CoolToneAudioProcessor& proc;
    juce::AudioFormatManager formats;
    juce::AudioBuffer<float> sourceAudio;
    std::vector<cooltone::Note> notes;
    std::vector<double> editedMidi;
    std::vector<std::pair<float,float>> waveformPeaks;
    juce::File loadedFile, renderedFile;
    double fileSampleRate = 44100.0;
    juce::Rectangle<float> noteArea;
    int selectedNote = -1;
    float dragStartY = 0.0f;
    double dragStartMidi = 0.0;
    bool fileLoaded = false, analysed = false, draggingNote = false;

    juce::ComboBox key, scale, preset;
    juce::TextButton magic{"MAGIC VIBRATO"}, vibratoButton{"VIBRATO"}, bypass{"BYPASS"};
    juce::TextButton openButton{"OPEN WAV"}, analyseButton{"ANALYZE NOTES"}, exportButton{"EXPORT TUNED"};
    ExternalFileDragButton dragButton{"HOLD & DRAG TO FL"};
    juce::Slider strength, retune, amount, rate, depth, output;
    juce::Label l1,l2,l3,l4,l5,l6;
    juce::LookAndFeel_V4 lf;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ka,sa;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ma,va,ba;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> a1,a2,a3,a4,a5,a6;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CoolToneAudioProcessorEditor)
};
