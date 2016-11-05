#ifndef _NoiseGateVst
#define _NoiseGateVst

#include "public.sdk/source/vst2.x/audioeffectx.h"
#include "NoiseGateKernel.h"

enum class Parameters
{
	// Gain Settings
	DetectorInput = 0,
	DetectorGain,

	// Noise Gate Settings
	ReductionDb,
	ThresholdDb,
	Slope,
	ReleaseMs,

	//CurrentGain,

	Count
};

using namespace NoiseInvader;

class NoiseGateVst : public AudioEffectX
{
public:
	NoiseGateVst(audioMasterCallback audioMaster);
	~NoiseGateVst();
	
	bool NoiseGateVst::getInputProperties(VstInt32 index, VstPinProperties* properties);
	bool NoiseGateVst::getOutputProperties(VstInt32 index, VstPinProperties* properties);

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
	int detectorInput;
	NoiseGateKernel* kernel;
};

#endif
