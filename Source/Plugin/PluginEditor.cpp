#include "PluginEditor.h"
#include "WavIO.h"
#include "ScaleEngine.h"
#include <cmath>
#include <algorithm>
#include <array>

namespace
{
const juce::Colour bg(0xff080a0f), panel(0xff121722), panel2(0xff1a2130),
    grid(0xff30394a), white(0xfff2f4fa), muted(0xff929caf),
    violet(0xffa78bfa), cyan(0xff69d8e8), green(0xff76e0ae), orange(0xffffbb70);
constexpr double pi = 3.14159265358979323846;
}

namespace
{
void pitchShiftNote (juce::AudioBuffer<float>& segment, double sampleRate,
                     double semitones, double vibratoCents, double vibratoRate)
{
    if (segment.getNumSamples() < 128) return;
    constexpr int ringSize = 16384;
    constexpr int minDelay = 512;
    const int range = juce::jlimit(256, 4096, static_cast<int>(sampleRate * 0.035));
    const int channels = segment.getNumChannels();
    const int length = segment.getNumSamples();
    const int total = length + minDelay + range + 8;
    std::vector<std::vector<float>> delay(static_cast<size_t>(channels),
                                          std::vector<float>(ringSize, 0.0f));
    std::vector<std::vector<float>> wet(static_cast<size_t>(channels),
                                        std::vector<float>(static_cast<size_t>(total), 0.0f));
    double phase = 0.0, vibratoPhase = 0.0;
    int write = 0;
    for (int n = 0; n < total; ++n)
    {
        const double vib = vibratoCents * std::sin(vibratoPhase);
        const double factor = std::pow(2.0, (semitones + vib / 100.0) / 12.0);
        vibratoPhase += 2.0 * pi * vibratoRate / sampleRate;
        if (vibratoPhase >= 2.0 * pi) vibratoPhase -= 2.0 * pi;
        phase += (1.0 - factor) / static_cast<double>(range);
        while (phase < 0.0) phase += 1.0;
        while (phase >= 1.0) phase -= 1.0;
        double phaseB = phase + 0.5;
        if (phaseB >= 1.0) phaseB -= 1.0;
        const double delayA = minDelay + phase * range;
        const double delayB = minDelay + phaseB * range;
        const float wa = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * pi * phase));
        for (int ch = 0; ch < channels; ++ch)
        {
            auto& line = delay[static_cast<size_t>(ch)];
            line[static_cast<size_t>(write)] = n < length ? segment.getSample(ch,n) : 0.0f;
            auto read = [&](double d)
            {
                double pos = static_cast<double>(write) - d;
                while (pos < 0.0) pos += ringSize;
                while (pos >= ringSize) pos -= ringSize;
                const int i = static_cast<int>(pos), j = (i + 1) % ringSize;
                const float frac = static_cast<float>(pos - std::floor(pos));
                return line[static_cast<size_t>(i)] +
                       frac * (line[static_cast<size_t>(j)] - line[static_cast<size_t>(i)]);
            };
            wet[static_cast<size_t>(ch)][static_cast<size_t>(n)] =
                read(delayA) * wa + read(delayB) * (1.0f - wa);
        }
        write = (write + 1) % ringSize;
    }
    for (int ch = 0; ch < channels; ++ch)
        for (int i = 0; i < length; ++i)
            segment.setSample(ch,i,wet[static_cast<size_t>(ch)][static_cast<size_t>(i+minDelay)]);
}
}


CoolToneAudioProcessorEditor::CoolToneAudioProcessorEditor(CoolToneAudioProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    formats.registerBasicFormats();
    setLookAndFeel(&lf);
    lf.setColour(juce::Slider::rotarySliderFillColourId, violet);
    lf.setColour(juce::Slider::rotarySliderOutlineColourId, grid);
    lf.setColour(juce::Slider::thumbColourId, white);
    lf.setColour(juce::Label::textColourId, muted);
    lf.setColour(juce::ComboBox::backgroundColourId, panel2);
    lf.setColour(juce::ComboBox::textColourId, white);
    lf.setColour(juce::ComboBox::outlineColourId, grid);
    lf.setColour(juce::TextButton::buttonColourId, panel2);
    lf.setColour(juce::TextButton::textColourOffId, white);

    key.addItemList({"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 1);
    scale.addItemList({"Major","Natural Minor","Harmonic Minor","Melodic Minor","Major Pentatonic","Minor Pentatonic","Blues","Jazz","Dorian","Phrygian","Lydian","Mixolydian","Chromatic","Custom"}, 1);
    preset.addItem("ORYMY SIGNATURE",1); preset.addItem("ROMANTIC BACHATA",2);
    preset.addItem("R&B EXPRESSIVE",3); preset.addItem("NATURAL VOCAL",4); preset.addItem("HARD TUNE",5);
    preset.onChange = [this]
    {
        const int id = preset.getSelectedId();
        auto set = [this](const char* n, float v)
        { if (auto* x = proc.state.getParameter(n)) x->setValueNotifyingHost(x->convertTo0to1(v)); };
        if (id == 2) { set("strength",.72f); set("retune",65); set("vibAmount",.68f); set("vibDepth",32); }
        else if (id == 3) { set("strength",.82f); set("retune",28); set("vibAmount",.52f); }
        else if (id == 4) { set("strength",.45f); set("retune",110); set("vibAmount",.3f); }
        else if (id == 5) { set("strength",1); set("retune",5); set("vibAmount",0); }
        else { set("strength",.85f); set("retune",35); set("vibAmount",.45f); }
    };

    auto cfg=[](juce::Slider& s)
    { s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag); s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,64,18); };
    for (auto* s : {&strength,&retune,&amount,&rate,&depth,&output}) cfg(*s);
    add(strength,l1,"CORRECTION"); add(retune,l2,"RETUNE"); add(amount,l3,"VIBRATO AMT");
    add(rate,l4,"VIBRATO RATE"); add(depth,l5,"VIBRATO DEPTH"); add(output,l6,"OUTPUT");

    for (auto* c : {&key,&scale,&preset}) addAndMakeVisible(*c);
    for (auto* b : {&magic,&vibratoButton,&bypass,&openButton,&analyseButton,&exportButton,&dragButton}) addAndMakeVisible(*b);
    magic.setClickingTogglesState(true); vibratoButton.setClickingTogglesState(true); vibratoButton.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff285b67)); magic.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff493c73));
    bypass.setClickingTogglesState(true);
    openButton.onClick = [this] { openAudio(); };
    analyseButton.onClick = [this] { analyseAudio(); };
    exportButton.onClick = [this] { if (exportTunedAudio()) dragButton.setEnabled(true); };
    dragButton.setEnabled(false);
    dragButton.onClick = [this]
    {
        if (renderedFile.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles({renderedFile.getFullPathName()}, false);
    };

    ka=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.state,"key",key);
    sa=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(proc.state,"scale",scale);
    ma=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.state,"magic",magic);
    va=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.state,"vibrato",vibratoButton);
    ba=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.state,"bypass",bypass);
    a1=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.state,"strength",strength);
    a2=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.state,"retune",retune);
    a3=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.state,"vibAmount",amount);
    a4=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.state,"vibRate",rate);
    a5=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.state,"vibDepth",depth);
    a6=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.state,"output",output);

    setResizable(true,true); setResizeLimits(1040,650,1920,1200); setSize(1280,800);
    startTimerHz(24);
}

CoolToneAudioProcessorEditor::~CoolToneAudioProcessorEditor() { setLookAndFeel(nullptr); }

void CoolToneAudioProcessorEditor::add(juce::Slider& s, juce::Label& l, const juce::String& t)
{
    addAndMakeVisible(s); l.setText(t,juce::dontSendNotification);
    l.setJustificationType(juce::Justification::centred); l.setFont(juce::Font(10,juce::Font::bold));
    addAndMakeVisible(l);
}

void CoolToneAudioProcessorEditor::openAudio()
{
    juce::FileChooser chooser("Open a vocal recording", juce::File{}, "*.wav;*.aif;*.aiff;*.flac");
    if (chooser.browseForFileToOpen()) loadAudioFile(chooser.getResult());
}

bool CoolToneAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files)
        if (f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".aif") ||
            f.endsWithIgnoreCase(".aiff") || f.endsWithIgnoreCase(".flac")) return true;
    return false;
}

void CoolToneAudioProcessorEditor::filesDropped(const juce::StringArray& files, int, int)
{
    if (!files.isEmpty()) loadAudioFile(juce::File(files[0]));
}

bool CoolToneAudioProcessorEditor::loadAudioFile(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
    if (!reader || reader->lengthInSamples <= 0 || reader->lengthInSamples > static_cast<juce::int64>(reader->sampleRate * 60.0 * 20.0))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"COOLTONE","Could not read this audio file, or it is longer than 20 minutes.");
        return false;
    }
    const int channels = juce::jlimit(1,2,static_cast<int>(reader->numChannels));
    const int length = static_cast<int>(reader->lengthInSamples);
    juce::AudioBuffer<float> loaded(channels,length);
    if (!reader->read(&loaded,0,length,0,true,true)) return false;
    sourceAudio = std::move(loaded);
    fileSampleRate = reader->sampleRate;
    loadedFile = file; fileLoaded = true; analysed = false; notes.clear(); editedMidi.clear();
    renderedFile = {};
    dragButton.setEnabled(false);
    analyseButton.setEnabled(true); exportButton.setEnabled(false);
    repaint();
    return true;
}

void CoolToneAudioProcessorEditor::analyseAudio()
{
    if (!fileLoaded || sourceAudio.getNumSamples() < 2048) return;
    std::vector<float> mono(static_cast<size_t>(sourceAudio.getNumSamples()),0.0f);
    for (int i=0;i<sourceAudio.getNumSamples();++i)
    {
        for (int c=0;c<sourceAudio.getNumChannels();++c) mono[static_cast<size_t>(i)] += sourceAudio.getSample(c,i);
        mono[static_cast<size_t>(i)] /= static_cast<float>(sourceAudio.getNumChannels());
    }
    cooltone::YinParams yp;
    yp.sampleRate = 44100.0;
    // The file rate is supplied by the reader's actual rate via the stored source metadata below.
    yp.sampleRate = fileSampleRate;
    yp.minHz = 65.0; yp.maxHz = 1100.0; yp.hopSeconds = 0.008;
    yp.threshold = 0.16; yp.maxAperiodicity = 0.34; yp.minRms = 0.0015;
    auto frames = cooltone::PitchDetector::analyse(mono.data(),mono.size(),yp);
    cooltone::SegmenterParams sp; sp.splitSemitones=0.62; sp.splitConfirmFrames=2;
    sp.minNoteSec=0.045; sp.smoothRadius=2; sp.maxGapFrames=2;
    notes = cooltone::segmentNotes(frames,yp.hopSeconds,sp);
    editedMidi.clear(); editedMidi.reserve(notes.size());
    cooltone::ScaleEngine scaleEngine;
    scaleEngine.setKey(key.getSelectedId()-1);
    scaleEngine.setScale(static_cast<cooltone::ScaleType>(juce::jlimit(0,13,scale.getSelectedId()-1)));
    const double correction=static_cast<double>(strength.getValue());
    for (const auto& n : notes) { const double snapped=scaleEngine.nearestAllowed(n.midiPitch); editedMidi.push_back(n.midiPitch+(snapped-n.midiPitch)*correction); }
    analysed = true; exportButton.setEnabled(!notes.empty()); repaint();
}

bool CoolToneAudioProcessorEditor::exportTunedAudio()
{
    if (!fileLoaded || !analysed || notes.empty()) return false;
    juce::AudioBuffer<float> rendered(sourceAudio);
    const int sr = juce::roundToInt(fileSampleRate);
    for (size_t ni=0; ni<notes.size(); ++ni)
    {
        const auto& note=notes[ni];
        const double semitones=editedMidi[ni]-note.midiPitch;
        if (std::abs(semitones)<0.015) continue;
        const int start=juce::jlimit(0,rendered.getNumSamples(),static_cast<int>(std::floor(note.startSec*sr)));
        const int end=juce::jlimit(start,rendered.getNumSamples(),static_cast<int>(std::ceil(note.endSec*sr)));
        const int len=end-start;
        if (len<512) continue;
        juce::AudioBuffer<float> segment(rendered.getNumChannels(),len);
        for(int c=0;c<rendered.getNumChannels();++c) segment.copyFrom(c,0,rendered,c,start,len);
        const bool useVibrato = vibratoButton.getToggleState() || magic.getToggleState();
        const double vibCents = useVibrato ? static_cast<double>(depth.getValue() * amount.getValue()) : 0.0;
        pitchShiftNote(segment,fileSampleRate,semitones,vibCents,rate.getValue());
        const int fade=juce::jmin(len/3,juce::jmax(32,sr/100));
        for(int c=0;c<rendered.getNumChannels();++c)
            for(int i=0;i<len;++i)
            {
                float w=1.0f;
                if(i<fade) w=static_cast<float>(0.5-0.5*std::cos(pi*i/fade));
                else if(i>len-fade) w=static_cast<float>(0.5-0.5*std::cos(pi*(len-i)/fade));
                const float dry=rendered.getSample(c,start+i), wet=segment.getSample(c,i);
                rendered.setSample(c,start+i,dry*(1.0f-w)+wet*w);
            }
    }

    cooltone::WavData wav; wav.sampleRate=sr; wav.channels=rendered.getNumChannels(); wav.format=cooltone::WavFormat::Float32;
    wav.samples.resize(static_cast<size_t>(rendered.getNumSamples())*static_cast<size_t>(wav.channels));
    for(int i=0;i<rendered.getNumSamples();++i)
        for(int c=0;c<wav.channels;++c)
            wav.samples[static_cast<size_t>(i)*static_cast<size_t>(wav.channels)+static_cast<size_t>(c)]=rendered.getSample(c,i);
    renderedFile=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("COOLTONE - "+loadedFile.getFileNameWithoutExtension()+" - tuned.wav");
    std::string error;
    if(!cooltone::writeWav(renderedFile.getFullPathName().toStdString(),wav,&error))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"COOLTONE export failed",juce::String(error));
        return false;
    }
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"COOLTONE","Tuned audio exported to Documents. Use DRAG TO FL to drop it into your DAW.");
    dragButton.setEnabled(true); return true;
}

int CoolToneAudioProcessorEditor::noteAtPosition(juce::Point<float> pos) const
{
    if(!noteArea.contains(pos) || notes.empty()) return -1;
    const double duration=static_cast<double>(sourceAudio.getNumSamples())/fileSampleRate;
    for(int i=static_cast<int>(notes.size())-1;i>=0;--i)
    {
        const auto& n=notes[static_cast<size_t>(i)];
        const float x1=noteArea.getX()+static_cast<float>(n.startSec/duration)*noteArea.getWidth();
        const float x2=noteArea.getX()+static_cast<float>(n.endSec/duration)*noteArea.getWidth();
        const float y=noteArea.getBottom()-static_cast<float>((editedMidi[static_cast<size_t>(i)]-48.0)/36.0)*noteArea.getHeight();
        if(pos.x>=x1 && pos.x<=x2 && std::abs(pos.y-y)<14.0f) return i;
    }
    return -1;
}

void CoolToneAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    selectedNote=noteAtPosition(e.position); draggingNote=selectedNote>=0;
    if(draggingNote){dragStartY=e.position.y;dragStartMidi=editedMidi[static_cast<size_t>(selectedNote)];repaint();}
}
void CoolToneAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if(!draggingNote || selectedNote<0 || noteArea.getHeight()<=0) return;
    const double delta=(dragStartY-e.position.y)*36.0/noteArea.getHeight();
    editedMidi[static_cast<size_t>(selectedNote)]=juce::jlimit(36.0,96.0,dragStartMidi+std::round(delta*2.0)/2.0);
    exportButton.setEnabled(true); dragButton.setEnabled(false); repaint();
}

void CoolToneAudioProcessorEditor::drawNoteEditor(juce::Graphics& g,juce::Rectangle<float> r)
{
    noteArea=r; g.setColour(panel);g.fillRoundedRectangle(r.expanded(12.0f,30.0f),10.0f);
    g.setColour(white);g.setFont(juce::Font(12,juce::Font::bold));g.drawText("NOTE EDITOR  /  DRAG NOTES UP OR DOWN TO RETUNE",r.getX(),r.getY()-25,r.getWidth(),18,juce::Justification::left);
    if(!fileLoaded)
    {g.setColour(muted);g.setFont(juce::Font(16));g.drawFittedText("DROP A VOCAL WAV HERE  OR  CLICK OPEN WAV",r.toNearestInt(),juce::Justification::centred,2);return;}
    const double duration=static_cast<double>(sourceAudio.getNumSamples())/fileSampleRate;
    for(int midi=48;midi<=84;++midi)
    {
        const float y=r.getBottom()-static_cast<float>(midi-48)/36.0f*r.getHeight();
        const int pc=midi%12; const bool black=(pc==1||pc==3||pc==6||pc==8||pc==10);
        g.setColour(black?juce::Colour(0xff171d29):juce::Colour(0xff202938));g.fillRect(r.getX(),y-1,r.getWidth(),1.0f);
        if(midi%12==0){g.setColour(muted);g.setFont(9.0f);g.drawText(juce::MidiMessage::getMidiNoteName(midi,true,true,3),r.getX()+3,y-12,34,12,juce::Justification::left);}
    }
    if(notes.empty()){g.setColour(orange);g.drawText("AUDIO LOADED — CLICK ANALYZE NOTES",r.toNearestInt(),juce::Justification::centred);return;}
    for(size_t i=0;i<notes.size();++i)
    {
        const auto& n=notes[i];const float x=r.getX()+static_cast<float>(n.startSec/duration)*r.getWidth();
        const float w=juce::jmax(4.0f,static_cast<float>((n.endSec-n.startSec)/duration)*r.getWidth());
        const float y=r.getBottom()-static_cast<float>(editedMidi[i]-48.0)/36.0f*r.getHeight();
        juce::Rectangle<float> b(x,y-7.0f,w,14.0f);
        g.setColour(static_cast<int>(i)==selectedNote?orange:cyan);g.fillRoundedRectangle(b,3.0f);
        g.setColour(juce::Colour(0xff10151e));g.setFont(9.0f);
        if(w>30)g.drawText(juce::MidiMessage::getMidiNoteName(static_cast<int>(std::lround(editedMidi[i])),true,true,3),b,juce::Justification::centred);
    }
    g.setColour(muted);g.setFont(10.0f);
    g.drawText(loadedFile.getFileName()+"  |  "+juce::String(notes.size())+" notes  |  "+juce::String(duration,1)+" sec",r.getX(),r.getBottom()+8,r.getWidth(),16,juce::Justification::left);
}

void CoolToneAudioProcessorEditor::timerCallback(){repaint();}
void CoolToneAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(bg);
    g.setColour(panel);g.fillRoundedRectangle(16,12,getWidth()-32.0f,58,10);
    g.setColour(white);g.setFont(juce::Font(24,juce::Font::bold));g.drawText("COOLTONE",30,20,170,30,juce::Justification::left);
    g.setColour(violet);g.setFont(juce::Font(10,juce::Font::bold));g.drawText("BY ORYMY  /  YMY RECORDS",198,29,220,18,juce::Justification::left);
    drawNoteEditor(g,noteArea);
    g.setColour(panel);g.fillRoundedRectangle(getWidth()*.72f,126,getWidth()*.26f,getHeight()-150.0f,12);
    g.setColour(white);g.setFont(juce::Font(12,juce::Font::bold));g.drawText("VOCAL PROCESSING",static_cast<int>(getWidth()*.72f+14),136,220,20,juce::Justification::left);
    const float p=proc.pitch();g.setColour(cyan);g.setFont(juce::Font(11,juce::Font::bold));
    g.drawText(p>0?"LIVE INPUT  "+juce::String(p,2)+" MIDI":"LIVE INPUT  --",30,getHeight()-28,250,18,juce::Justification::left);
}

void CoolToneAudioProcessorEditor::resized()
{
    const int w=getWidth(),h=getHeight();
    preset.setBounds(420,22,170,34);key.setBounds(600,22,70,34);scale.setBounds(680,22,145,34);
    magic.setBounds(w-420,22,130,34);vibratoButton.setBounds(w-280,22,105,34);bypass.setBounds(w-165,22,130,34);
    openButton.setBounds(24,88,120,30);analyseButton.setBounds(154,88,150,30);
    exportButton.setBounds(314,88,140,30);dragButton.setBounds(464,88,125,30);
    const int right=static_cast<int>(w*.73f),kw=(w-right-30)/2,top=180,kh=145;
    auto pos=[&](juce::Slider& s,juce::Label& l,int col,int row)
    {const int x=right+col*kw,y=top+row*kh;s.setBounds(x+5,y,kw-10,105);l.setBounds(x+2,y+106,kw-4,20);};
    pos(strength,l1,0,0);pos(retune,l2,1,0);pos(amount,l3,0,1);
    pos(rate,l4,1,1);pos(depth,l5,0,2);pos(output,l6,1,2);
    noteArea={28.0f,162.0f,static_cast<float>(right-48),static_cast<float>(h-220)};
}
