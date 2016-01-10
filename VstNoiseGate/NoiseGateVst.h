#ifndef _NoiseGateVst
#define _NoiseGateVst

#include "public.sdk/source/vst2.x/audioeffectx.h"
#include "NoiseGateKernel.h"

enum class Parameters
{
	Enabled = 0,

	// Gain Settings
	InputGain,
	OutputGain,

	// Envelope Settings
	AttackMs,
	ReleaseMs,

	// expander settings
	SignalFloor,
	RatioOpen,
	RatioClose,
	ThresholdOpenDb,
	ThresholdCloseRelativeDb,
	KneeDb,

	// signal shaping
	//LowpassHz,
	//HighpassHz,

	Count
};

class NoiseGateVst : public AudioEffectX
{
public:
	NoiseGateVst(audioMasterCallback audioMaster);
	~NoiseGateVst();
	
	// Programs
	virtual void setProgramName (char* name);
	virtual void getProgramName (char* name);

	// Parameters
	virtual void setParameter (VstInt32 index, float value);
	virtual float getParameter (VstInt32 index);
	virtual void getParameterLabel (VstInt32 index, char* label);
	virtual void getParameterDisplay (VstInt32 index, char* text);
	virtual void getParameterName (VstInt32 index, char* text);

	// Metadata
	virtual bool getEffectName (char* name);
	virtual bool getVendorString (char* text);
	virtual bool getProductString (char* text);
	virtual VstInt32 getVendorVersion ();

	// Processing
	virtual void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames);
	virtual void setSampleRate(float sampleRate);
	void createDevice();

protected:
	float parameters[(int)Parameters::Count];
	char programName[kVstMaxProgNameLen + 1];
	NoiseGateKernel* kernel;
};

#endif
