#include "NoiseGateVst.h"
#include "AudioLib/Utils.h"
#include "AudioLib/ValueTables.h"

using namespace AudioLib;

const char* PluginName = "Noise Invader";
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

	parameters[(int)Parameters::Enabled] = 1.0;
	parameters[(int)Parameters::AttackMs] = 0.1;
	parameters[(int)Parameters::InputGain] = 0.5;
	parameters[(int)Parameters::KneeDb] = 0.5;
	parameters[(int)Parameters::OutputGain] = 0.5;
	parameters[(int)Parameters::SignalFloor] = 0.0;
	parameters[(int)Parameters::RatioOpen] = 0.4;
	parameters[(int)Parameters::RatioClose] = 0.4;
	parameters[(int)Parameters::ReleaseMs] = 0.4;
	parameters[(int)Parameters::ThresholdCloseRelativeDb] = 0.1;
	parameters[(int)Parameters::ThresholdOpenDb] = 0.3;

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
	case Parameters::Enabled:
		kernel->Enabled = value >= 0.5;
		break;
	case Parameters::InputGain:
		kernel->InputGain = Utils::DB2gain(-20 + value * 40);
		break;
	case Parameters::OutputGain:
		kernel->OutputGain = Utils::DB2gain(-20 + value * 40);
		break;
	case Parameters::AttackMs:
		kernel->AttackMs = 1 + ValueTables::Get(value, ValueTables::Response2Dec) * 299;
		break;
	case Parameters::ReleaseMs:
		kernel->ReleaseMs = 10 + ValueTables::Get(value, ValueTables::Response2Dec) * 990;
		break;
	case Parameters::KneeDb:
		kernel->KneeDb = 0.01 + value * value * 12.0;
		break;
	case Parameters::SignalFloor:
		kernel->SignalFloor = -50 - value * 100;
		break;
	case Parameters::RatioOpen:
		kernel->RatioOpen = 1 + ValueTables::Get(value, ValueTables::Response2Dec) * 19;
		break;
	case Parameters::RatioClose:
		kernel->RatioClose = 1 + ValueTables::Get(value, ValueTables::Response2Dec) * 19;
		break;
	case Parameters::ThresholdOpenDb:
		kernel->ThresholdOpenDb = -ValueTables::Get(1 - parameters[(int)Parameters::ThresholdOpenDb], ValueTables::Response2Oct) * 80;
		kernel->ThresholdCloseDb = kernel->ThresholdOpenDb - ValueTables::Get(1 - parameters[(int)Parameters::ThresholdCloseRelativeDb], ValueTables::Response2Oct) * 20;
		// since we're changing more than one parameter, need to notify the host
		setParameterAutomated((int)Parameters::ThresholdCloseRelativeDb, parameters[(int)Parameters::ThresholdCloseRelativeDb]);
		//updateDisplay();
		break;
	case Parameters::ThresholdCloseRelativeDb:
		kernel->ThresholdCloseDb = kernel->ThresholdOpenDb - ValueTables::Get(1 - parameters[(int)Parameters::ThresholdCloseRelativeDb], ValueTables::Response2Oct) * 20;		
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
	case Parameters::Enabled:
		strcpy(label, "Enabled");
		break;
	case Parameters::InputGain:
		strcpy(label, "Input Gain");
		break;
	case Parameters::OutputGain:
		strcpy(label, "Output Gain");
		break;
	case Parameters::AttackMs:
		strcpy(label, "Attack");
		break;
	case Parameters::ReleaseMs:
		strcpy(label, "Release");
		break;
	case Parameters::KneeDb:
		strcpy(label, "Knee");
		break;
	case Parameters::SignalFloor:
		strcpy(label, "Signal Floor");
		break;
	case Parameters::RatioOpen:
		strcpy(label, "Ratio Open");
		break;
	case Parameters::RatioClose:
		strcpy(label, "Ratio Close");
		break;
	case Parameters::ThresholdOpenDb:
		strcpy(label, "Thresh. Open");
		break;
	case Parameters::ThresholdCloseRelativeDb:
		strcpy(label, "Thresh. Close");
		break;
	}
}

void NoiseGateVst::getParameterDisplay(VstInt32 index, char* text)
{
	switch ((Parameters)index)
	{
	case Parameters::Enabled:
		strcpy(text, kernel->Enabled ? "Enabled" : "Disabled");
		break;
	case Parameters::InputGain:
		sprintf(text, "%.2f", Utils::Gain2DB(kernel->InputGain));
		break;
	case Parameters::OutputGain:
		sprintf(text, "%.2f", Utils::Gain2DB(kernel->OutputGain));
		break;
	case Parameters::AttackMs:
		sprintf(text, "%.1f", kernel->AttackMs);
		break;
	case Parameters::ReleaseMs:
		sprintf(text, "%.1f", kernel->ReleaseMs);
		break;
	case Parameters::KneeDb:
		sprintf(text, "%.1f", kernel->KneeDb);
		break;
	case Parameters::SignalFloor:
		sprintf(text, "%i", (int)kernel->SignalFloor);
		break;
	case Parameters::RatioOpen:
		sprintf(text, "%.1f", kernel->RatioOpen);
		break;
	case Parameters::RatioClose:
		sprintf(text, "%.1f", kernel->RatioClose);
		break;
	case Parameters::ThresholdOpenDb:
		sprintf(text, "%.1f", kernel->ThresholdOpenDb);
		break;
	case Parameters::ThresholdCloseRelativeDb:
		sprintf(text, "%.1f", kernel->ThresholdCloseDb);
		break;
	}
}

void NoiseGateVst::getParameterLabel(VstInt32 index, char* label)
{
	switch ((Parameters)index)
	{
	case Parameters::Enabled:
		strcpy(label, "");
		break;
	case Parameters::InputGain:
		strcpy(label, "dB");
		break;
	case Parameters::OutputGain:
		strcpy(label, "dB");
		break;
	case Parameters::AttackMs:
		strcpy(label, "ms");
		break;
	case Parameters::ReleaseMs:
		strcpy(label, "ms");
		break;
	case Parameters::KneeDb:
		strcpy(label, "dB");
		break;
	case Parameters::SignalFloor:
		strcpy(label, "dB");
		break;
	case Parameters::RatioOpen:
	case Parameters::RatioClose:
		strcpy(label, "");
		break;
	case Parameters::ThresholdOpenDb:
	case Parameters::ThresholdCloseRelativeDb:
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
	return 1000; 
}

void NoiseGateVst::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames)
{
    float* in  = inputs[0];
    float* out = outputs[0];
    
	kernel->Process(in, out, sampleFrames);
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
	kernel->LowpassHz = 4000.0f;
	kernel->HighpassHz = 60.0f;

	// re-apply parameters
	for (size_t i = 0; i < (int)Parameters::Count; i++)
	{
		setParameter(i, parameters[i]);
	}
}

