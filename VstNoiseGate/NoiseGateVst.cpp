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
	detectorInput = 0;
	setNumInputs(4); // 2x stereo in, only left channel used on second input
	setNumOutputs(2); // 1x stereo out
	setUniqueID(91572785); // Random ID
	canProcessReplacing();
	canDoubleReplacing(false);
	isSynth(false);
	vst_strncpy (programName, PluginName, kVstMaxProgNameLen);

	kernel = 0;
	sampleRate = 48000;

	parameters[(int)Parameters::DetectorInput] = 0.0;
	parameters[(int)Parameters::DetectorGain] = 0.5;
	parameters[(int)Parameters::ReductionDb] = 0.5;
	parameters[(int)Parameters::ThresholdDb] = 0.8;
	parameters[(int)Parameters::Slope] = 0.5;
	parameters[(int)Parameters::ReleaseMs] = 0.3;
	//parameters[(int)Parameters::CurrentGain] = 0.0;
	
	createDevice();
}

NoiseGateVst::~NoiseGateVst()
{
	delete kernel;
	kernel = 0;
}

bool NoiseGateVst::getInputProperties(VstInt32 index, VstPinProperties* properties)
{
	if (index == 0) // 0-1 = Main in
	{
		properties->arrangementType = kSpeakerArrStereo;
		properties->flags = kVstPinIsStereo;
		sprintf(properties->shortLabel, "Main In");
		sprintf(properties->label, "Main Input");
		return true;
	}
	else if (index == 2) // 2-3 = Aux in
	{
		properties->arrangementType = kSpeakerArrStereo;
		properties->flags = kVstPinIsStereo;
		sprintf(properties->shortLabel, "Aux");
		sprintf(properties->label, "Aux Input");
		return true;
	}
	else
	{
		return false;
	}
}

bool NoiseGateVst::getOutputProperties(VstInt32 index, VstPinProperties* properties)
{
	if (index == 0) // 0-1 = Main out
	{
		properties->arrangementType = kSpeakerArrStereo;
		properties->flags = kVstPinIsStereo;
		sprintf(properties->shortLabel, "Output");
		sprintf(properties->label, "Main Output");
		return true;
	}
	else
	{
		return false;
	}
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
	bool update = true;

	switch ((Parameters)index)
	{
	case Parameters::DetectorInput:
		detectorInput = (int)(value * 1.999);
		break;
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
	default:
		update = false;
	}

	if (update)
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
	case Parameters::DetectorInput:
		strcpy(label, "Detection");
		break;
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

	// for readout only
	/*case Parameters::CurrentGain:
		strcpy(label, "Output Gain");
		break;*/
	}
}

void NoiseGateVst::getParameterDisplay(VstInt32 index, char* text)
{
	switch ((Parameters)index)
	{
	case Parameters::DetectorInput:
		if (detectorInput == 0)
			sprintf(text, "Main Left");
		else if (detectorInput == 1)
			sprintf(text, "Aux Left");
		else
			sprintf(text, "-----");
		break;
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
	/*case Parameters::CurrentGain:
		sprintf(text, "%.7f", kernel->currentGainDb);
		break;*/
	}
}

void NoiseGateVst::getParameterLabel(VstInt32 index, char* label)
{
	switch ((Parameters)index)
	{
	case Parameters::DetectorInput:
		strcpy(label, "");
		break;
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
	//case Parameters::CurrentGain:
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
    float* inL  = inputs[0];
	float* inR = inputs[1];

    float* outL = outputs[0];
	float* outR = outputs[1];

	float* detector = detectorInput == 0 ? inputs[0] : inputs[2];
    
	kernel->Process(inL, inR, detector, outL, outR, sampleFrames);
	//setParameterAutomated((int)Parameters::CurrentGain, kernel->currentGainDb / 150 + 1);
}

void NoiseGateVst::setSampleRate(float sampleRate)
{
	this->sampleRate = sampleRate;
	createDevice();
}

void NoiseGateVst::createDevice()
{
	delete kernel;
	kernel = new NoiseGateKernel(sampleRate);

	// re-apply parameters
	for (size_t i = 0; i < (int)Parameters::Count; i++)
	{
		setParameter(i, parameters[i]);
	}
}

