#include "NoiseGateVst.h"
#include "AudioLib/Utils.h"
#include "AudioLib/ValueTables.h"

using namespace AudioLib;

const char* PluginName = "Noise Invader v2.0";
const char* DeveloperName = "Valdemar Erlingsson";

AudioEffect* createEffectInstance (audioMasterCallback audioMaster)
{
	Utils::Initialize();
	ValueTables::Init();
	return new NoiseGateVst(audioMaster);
}

// ------------------------------------------------------------------------------------

NoiseGateVst::NoiseGateVst(audioMasterCallback audioMaster)
	: AudioEffectX (audioMaster, 1, (int)Parameters::Count)
{
	setNumInputs(1); // Mono in
	setNumOutputs(1); // Mono out
	setUniqueID(91572785); // Random ID
	canProcessReplacing();
	canDoubleReplacing(false);
	isSynth(false);
	vst_strncpy (programName, PluginName, kVstMaxProgNameLen);

	kernel = 0;
	sampleRate = 48000;

	parameters[(int)Parameters::DetectorGain] = 0.5;
	parameters[(int)Parameters::ReductionDb] = 0.5;
	parameters[(int)Parameters::ThresholdDb] = 0.8;
	parameters[(int)Parameters::Slope] = 0.5;
	parameters[(int)Parameters::ReleaseMs] = 0.3;
	
	createDevice();
}

NoiseGateVst::~NoiseGateVst()
{
	delete kernel;
	kernel = 0;
}

void NoiseGateVst::setProgramName(char* name)
{
	vst_strncpy(programName, name, kVstMaxProgNameLen);
}

void NoiseGateVst::getProgramName(char* name)
{
	vst_strncpy(name, programName, kVstMaxProgNameLen);
}

void NoiseGateVst::setParameter(VstInt32 index, float value)
{
	parameters[index] = value;

	switch ((Parameters)index)
	{
	case Parameters::DetectorGain:
		kernel->DetectorGain = Utils::DB2gain(40 * value - 20);
		break;
	case Parameters::ReductionDb:
		kernel->ReductionDb = -value * 100;
		break;
	case Parameters::ReleaseMs:
		kernel->ReleaseMs = 10 + ValueTables::Get(value, ValueTables::Response2Dec) * 990;
		break;
	case Parameters::Slope:
		kernel->Slope = 1.0f + ValueTables::Get(value, ValueTables::Response2Dec) * 50;
		break;
	case Parameters::ThresholdDb:
		kernel->ThresholdDb = -ValueTables::Get(1 - value, ValueTables::Response2Oct) * 80;
		break;
	}

	kernel->UpdateAll();
}

float NoiseGateVst::getParameter(VstInt32 index)
{
	return parameters[index];
}

void NoiseGateVst::getParameterName(VstInt32 index, char* label)
{
	switch ((Parameters)index)
	{
	case Parameters::DetectorGain:
		strcpy(label, "Sensitivity");
		break;
	case Parameters::ReductionDb:
		strcpy(label, "Reduction");
		break;
	case Parameters::ReleaseMs:
		strcpy(label, "Release");
		break;
	case Parameters::Slope:
		strcpy(label, "Slope");
		break;
	case Parameters::ThresholdDb:
		strcpy(label, "Threshold");
		break;
	}
}

void NoiseGateVst::getParameterDisplay(VstInt32 index, char* text)
{
	switch ((Parameters)index)
	{
	case Parameters::DetectorGain:
		sprintf(text, "%.2f", Utils::Gain2DB(kernel->DetectorGain));
		break;
	case Parameters::ReductionDb:
		sprintf(text, "%.1f", kernel->ReductionDb);
		break;
	case Parameters::ReleaseMs:
		sprintf(text, "%.1f", kernel->ReleaseMs);
		break;
	case Parameters::Slope:
		sprintf(text, "%.2f", kernel->Slope);
		break;
	case Parameters::ThresholdDb:
		sprintf(text, "%.1f", kernel->ThresholdDb);
		break;
	}
}

void NoiseGateVst::getParameterLabel(VstInt32 index, char* label)
{
	switch ((Parameters)index)
	{
	case Parameters::DetectorGain:
		strcpy(label, "dB");
		break;
	case Parameters::ReductionDb:
		strcpy(label, "dB");
		break;
	case Parameters::ReleaseMs:
		strcpy(label, "ms");
		break;
	case Parameters::Slope:
		strcpy(label, "");
		break;
	case Parameters::ThresholdDb:
		strcpy(label, "dB");
		break;
	}
}

bool NoiseGateVst::getEffectName(char* name)
{
	vst_strncpy(name, PluginName, kVstMaxEffectNameLen);
	return true;
}

bool NoiseGateVst::getProductString(char* text)
{
	vst_strncpy(text, PluginName, kVstMaxProductStrLen);
	return true;
}

bool NoiseGateVst::getVendorString(char* text)
{
	vst_strncpy(text, DeveloperName, kVstMaxVendorStrLen);
	return true;
}

VstInt32 NoiseGateVst::getVendorVersion()
{ 
	return 2000; 
}

void NoiseGateVst::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames)
{
    float* in  = inputs[0];
    float* out = outputs[0];
    
	kernel->Process(in, in, out, sampleFrames);
}

void NoiseGateVst::setSampleRate(float sampleRate)
{
	this->sampleRate = sampleRate;
	createDevice();
}

void NoiseGateVst::createDevice()
{
	delete kernel;
	kernel = new NoiseGateKernel2(sampleRate);

	// re-apply parameters
	for (size_t i = 0; i < (int)Parameters::Count; i++)
	{
		setParameter(i, parameters[i]);
	}
}

