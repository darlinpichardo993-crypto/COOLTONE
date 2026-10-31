#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
namespace {constexpr double PI=3.14159265358979323846;constexpr int DS=16384,MIN_D=512;
float get(juce::AudioProcessorValueTreeState&s,const char*n,float d=0){auto*p=s.getRawParameterValue(n);return p?p->load():d;}}
juce::AudioProcessorValueTreeState::ParameterLayout CoolToneAudioProcessor::makeParams(){
 using namespace juce;std::vector<std::unique_ptr<RangedAudioParameter>> p;
 p.push_back(std::make_unique<AudioParameterFloat>("strength","Pitch Strength",NormalisableRange<float>(0,1),.85f));
 p.push_back(std::make_unique<AudioParameterFloat>("retune","Retune Speed",NormalisableRange<float>(5,150),35));
 p.push_back(std::make_unique<AudioParameterChoice>("key","Key",StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"},0));
 p.push_back(std::make_unique<AudioParameterChoice>("scale","Scale",StringArray{"Major","Natural Minor","Harmonic Minor","Melodic Minor","Major Pentatonic","Minor Pentatonic","Blues","Jazz","Dorian","Phrygian","Lydian","Mixolydian","Chromatic","Custom"},0));
 p.push_back(std::make_unique<AudioParameterBool>("magic","Magic Vibrato",false));
 p.push_back(std::make_unique<AudioParameterBool>("vibrato","Vibrato",false));
 p.push_back(std::make_unique<AudioParameterFloat>("vibAmount","Vibrato Amount",NormalisableRange<float>(0,1),.70f));
 p.push_back(std::make_unique<AudioParameterFloat>("vibRate","Vibrato Rate",NormalisableRange<float>(2,9),5.5f));
 p.push_back(std::make_unique<AudioParameterFloat>("vibDepth","Vibrato Depth",NormalisableRange<float>(0,100),35));
 p.push_back(std::make_unique<AudioParameterFloat>("output","Output",NormalisableRange<float>(-18,6),0));
 p.push_back(std::make_unique<AudioParameterBool>("bypass","Bypass",false));return{p.begin(),p.end()};
}
CoolToneAudioProcessor::CoolToneAudioProcessor():AudioProcessor(BusesProperties().withInput("Input",AudioChannelSet::stereo(),true).withOutput("Output",AudioChannelSet::stereo(),true)),state(*this,nullptr,"COOLTONE",makeParams()){for(auto&v:trace)v.store(-1);}
void CoolToneAudioProcessor::prepareToPlay(double sr,int){sampleRate=sr;delay.resize(static_cast<size_t>(juce::jmax(1,getTotalNumOutputChannels())));for(auto&d:delay)d.assign(DS,0);ring.fill(0);write=delayWrite=analysisCount=stable=0;phase=vibPhase=0;ratio=1;targetShift=0;setLatencySamples(MIN_D);}
void CoolToneAudioProcessor::releaseResources(){for(auto&d:delay)std::fill(d.begin(),d.end(),0);}
bool CoolToneAudioProcessor::isBusesLayoutSupported(const BusesLayout&l)const{auto a=l.getMainInputChannelSet(),b=l.getMainOutputChannelSet();return a==b&&(b==juce::AudioChannelSet::mono()||b==juce::AudioChannelSet::stereo());}
void CoolToneAudioProcessor::analyse()noexcept{
 constexpr int N=1536;std::array<float,N>x{};double mean=0;
 for(int i=0;i<N;++i){x[static_cast<size_t>(i)]=ring[static_cast<size_t>((write-N+i+2048)%2048)];mean+=x[static_cast<size_t>(i)];}mean/=N;
 double e=0;for(auto&v:x){v=static_cast<float>(v-mean);e+=v*v;}if(e/N<.000012){currentPitch.store(-1);stable=0;targetShift=0;return;}
 int lo=juce::jmax(2,static_cast<int>(sampleRate/1000)),hi=juce::jmin(N/2,static_cast<int>(sampleRate/70)),bestLag=0;double best=-1;
 for(int lag=lo;lag<=hi;++lag){double c=0,a=0,b=0;for(int i=0;i<N-lag;i+=2){double u=x[static_cast<size_t>(i)],v=x[static_cast<size_t>(i+lag)];c+=u*v;a+=u*u;b+=v*v;}double q=(a>1e-12&&b>1e-12)?c/std::sqrt(a*b):0;if(q>best){best=q;bestLag=lag;}}
 if(bestLag==0||best<.60){currentPitch.store(-1);stable=0;targetShift=0;return;}
 auto corrAt=[&](int lag)
 {
  double c=0,a=0,b=0;
  for(int i=0;i<N-lag;i+=2){double u=x[static_cast<size_t>(i)],v=x[static_cast<size_t>(i+lag)];c+=u*v;a+=u*u;b+=v*v;}
  return (a>1e-12&&b>1e-12)?c/std::sqrt(a*b):0.0;
 };
 double lagF=static_cast<double>(bestLag);
 if(bestLag>lo&&bestLag<hi)
 {
  const double q0=corrAt(bestLag-1),q1=corrAt(bestLag),q2=corrAt(bestLag+1);
  const double den=q0-2.0*q1+q2;
  if(std::abs(den)>1.0e-9) lagF+=juce::jlimit(-0.5,0.5,0.5*(q0-q2)/den);
 }
 double midi=69+12*std::log2((sampleRate/lagF)/440.0);float old=currentPitch.load();stable=(old>0&&std::abs(midi-old)<.45f)?stable+512:0;currentPitch.store(static_cast<float>(midi));
 scales.setKey(static_cast<int>(get(state,"key")));scales.setScale(static_cast<cooltone::ScaleType>(juce::jlimit(0,13,static_cast<int>(get(state,"scale")))));
 targetShift=static_cast<float>((scales.nearestAllowed(midi)-midi)*get(state,"strength",.85f));
 int k=tracePos.fetch_add(1)%256;trace[static_cast<size_t>(k)].store(static_cast<float>(midi));
}
void CoolToneAudioProcessor::processBlock(juce::AudioBuffer<float>&b,juce::MidiBuffer&){
 juce::ScopedNoDenormals guard;int nc=b.getNumChannels(),ns=b.getNumSamples();bool bypass=get(state,"bypass")>.5f,magic=get(state,"magic")>.5f,manualVibrato=get(state,"vibrato")>.5f;
 float rate=get(state,"vibRate",5.5f),depth=get(state,"vibDepth",22)*get(state,"vibAmount"),gain=juce::Decibels::decibelsToGain(get(state,"output"));
 double range=juce::jmax(128.0,sampleRate*.035),smooth=1-std::exp(-1.0/(juce::jmax(.005,get(state,"retune",35)*.001)*sampleRate));
 for(int n=0;n<ns;++n){float mono=0;for(int c=0;c<nc;++c)mono+=b.getSample(c,n);mono/=static_cast<float>(juce::jmax(1,nc));ring[static_cast<size_t>(write)]=mono;write=(write+1)%2048;if(++analysisCount>=512){analysisCount=0;analyse();}
  double vib=((manualVibrato||(magic&&stable>sampleRate*.22)))?depth*std::sin(vibPhase):0;vibPhase+=2*PI*rate/sampleRate;if(vibPhase>2*PI)vibPhase-=2*PI;
  float wanted=std::pow(2.0f,(targetShift+static_cast<float>(vib/100))/12.0f);ratio+=static_cast<float>(smooth)*(wanted-ratio);
  phase+=(1.0-ratio)/range;while(phase<0)phase+=1;while(phase>=1)phase-=1;
  for(int c=0;c<nc;++c){float in=b.getSample(c,n);auto&d=delay[static_cast<size_t>(c)];d[static_cast<size_t>(delayWrite)]=in;
   double p2=phase+.5;if(p2>=1)p2-=1;double da=MIN_D+phase*range,db=MIN_D+p2*range,wa=.5+.5*std::cos(2*PI*phase);
   auto read=[&](double dl){double pos=static_cast<double>(delayWrite)-dl;while(pos<0)pos+=DS;while(pos>=DS)pos-=DS;int i=static_cast<int>(pos),j=(i+1)%DS;float f=static_cast<float>(pos-std::floor(pos));return d[static_cast<size_t>(i)]+f*(d[static_cast<size_t>(j)]-d[static_cast<size_t>(i)]);};
   float out=static_cast<float>(read(da)*wa+read(db)*(1-wa));b.setSample(c,n,bypass?in:out*gain);
  }
  delayWrite=(delayWrite+1)%DS;
 }
}
void CoolToneAudioProcessor::getStateInformation(juce::MemoryBlock&d){auto s=state.copyState();std::unique_ptr<juce::XmlElement>x(s.createXml());copyXmlToBinary(*x,d);}
void CoolToneAudioProcessor::setStateInformation(const void*d,int n){std::unique_ptr<juce::XmlElement>x(getXmlFromBinary(d,n));if(x&&x->hasTagName(state.state.getType()))state.replaceState(juce::ValueTree::fromXml(*x));}
float CoolToneAudioProcessor::history(int age)const{int w=tracePos.load(),i=(w-1-juce::jlimit(0,255,age)+256)%256;return trace[static_cast<size_t>(i)].load();}
juce::AudioProcessorEditor* CoolToneAudioProcessor::createEditor(){return new CoolToneAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new CoolToneAudioProcessor();}
