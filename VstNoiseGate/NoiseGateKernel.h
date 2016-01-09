#ifndef _NoiseGateKernel
#define _NoiseGateKernel

#include <iostream>
#include <cmath>
#include "AudioLib/MathDefs.h"
#include "AudioLib/Biquad.h"
#include "AudioLib/Utils.h"
#include "PeakDetector.h"
#include "HumEliminator.h"

using namespace AudioLib;

class NoiseGateKernel
{
private:

	const float MinEnvValDb = -150.0f;

	float fs;
		
	Biquad lowpass;
	Biquad highpass;
	PeakDetector detector;

	// temp values
	float envValDb1Filter;
	float envValDb2Filter;
	float envValDb;
	float aPrevDb;
	
	// envelope settings - internal
	float attackSlew;
	float releaseSlew;
	float envelopeCutoffHz;
	float envelopeAlpha;

public:
	
	HumEliminator HumEliminator;

	// Gain Settings
	float InputGain;
	float OutputGain;

	// Envelope Settings
	float AttackMs;
	float ReleaseMs;

	// expander settings
	float Ratio;
	float ThresholdDb;
	float KneeDb;

	// signal shaping
	float LowpassHz;
	float HighpassHz;

	NoiseGateKernel(int fs)
		: detector(fs), HumEliminator(fs)
	{
		this->fs = fs;

		HumEliminator.Mode = 0;
		envValDb1Filter = MinEnvValDb;
		envValDb2Filter = MinEnvValDb;
		envValDb = MinEnvValDb;
		aPrevDb = MinEnvValDb;
		attackSlew = 0;
		releaseSlew = 0;
		envelopeCutoffHz = 100.0f; // Adjustable?
		envelopeAlpha = 0;
		
		InputGain = 1.0f;
		OutputGain = 1.0f;

		AttackMs = 5.f;
		ReleaseMs = 50.f;

		Ratio = 3.0f;
		ThresholdDb = -20.f;
		KneeDb = 0.01;

		LowpassHz = 6000.f;
		HighpassHz = 50.f;

		lowpass.SetSamplerate(fs);
		highpass.SetSamplerate(fs);
		lowpass.Type = Biquad::FilterType::LowPass;
		highpass.Type = Biquad::FilterType::HighPass;
		lowpass.SetQ(1.0f);
		highpass.SetQ(1.0f);
		
		UpdateAll();
	}

	inline ~NoiseGateKernel()
	{

	}

	inline void Process(float* input, float* output, int len)
	{
		HumEliminator.Process(input, input, len);

		UpdateAll();
		float g = 1.0f;

		for (size_t i = 0; i < len; i++)
		{
			float x = lowpass.Process(input[i] * InputGain);
			x = highpass.Process(x);
			float absVal = abs(x);

			// ------ Peak tracking --------
			auto peakVal = detector.ProcessPeaks(x);

			// ------ Env Shaping --------

			auto peakValDb = Utils::Gain2DB(peakVal);

			if (envValDb < peakValDb)
			{
				envValDb += attackSlew;
				if (envValDb > peakValDb)
					envValDb = peakValDb;
			}
			else
			{
				envValDb -= releaseSlew;
				if (envValDb < peakValDb)
					envValDb = peakValDb;
			}

			if (envValDb < MinEnvValDb)
				envValDb = MinEnvValDb;

			envValDb1Filter = envValDb1Filter * (1 - envelopeAlpha) + envValDb * envelopeAlpha;
			envValDb2Filter = envValDb2Filter * (1 - envelopeAlpha) + envValDb1Filter * envelopeAlpha;

			// the assumed gain
			auto aDb = Compress(envValDb2Filter, ThresholdDb, Ratio, KneeDb, true);

			// slew limit the effective gain curve with the same attack and release params
			// as the envelope follower
			if (aDb < MinEnvValDb)
				aDb = MinEnvValDb;
			if (aDb > aPrevDb)
			{
				if (aPrevDb + attackSlew < aDb)
					aDb = aPrevDb + attackSlew;
			}
			else
			{
				if (aPrevDb - releaseSlew > aDb)
					aDb = aPrevDb - releaseSlew;
			}
			aPrevDb = aDb;

			auto cValue = Utils::DB2gain(aDb);
			auto cEnvValue = Utils::DB2gain(envValDb2Filter);
			auto g = cValue / cEnvValue;

			output[i] = input[i] * g * OutputGain;
			//output[i] = x * OutputGain;
		}

		//std::cout << "Measured: " << envelopeDbValue << ", g: " << g << "\n";
	}

private:

	inline void UpdateAll()
	{
		attackSlew = (100 / fs) / (AttackMs * 0.001); // 100 dB movement in a given time period
		releaseSlew = (100 / fs) / (ReleaseMs * 0.001);

		// https://en.wikipedia.org/wiki/Low-pass_filter
		float k = 2 * M_PI * envelopeCutoffHz / fs;
		envelopeAlpha = (k) / (k + 1);

		lowpass.Frequency = LowpassHz;
		highpass.Frequency = HighpassHz;
		lowpass.Update();
		highpass.Update();
	}

	// Given x, computes the compression/expansion curve according to the parameters specified
	static float Compress(float x, float threshold, float ratio, float knee, bool expand)
	{
		// the assumed gain
		auto output = x;
		auto kneeLow = threshold - knee;
		auto kneeHigh = threshold + knee;

		if (x <= kneeLow)
		{
			output = x;
		}
		else if (x >= kneeHigh)
		{
			auto diff = x - threshold;
			auto a = threshold + diff / ratio;
			output = a;
		}
		else // in knee, below threshold
		{
			// position on the interpolating line between the two parts of the compression curve
			auto positionOnLine = (x - kneeLow) / (kneeHigh - kneeLow);
			auto kDiff = knee * positionOnLine;
			auto xa = kneeLow + kDiff;
			auto ya = xa;
			auto yb = threshold + kDiff / ratio;
			auto slope = (yb - ya) / (knee);
			output = xa + slope * positionOnLine * knee;
		}

		// if doing expansion, adjust the slopes so that instead of compressing, the curve expands
		if (expand)
		{
			// to expand, we multiple the output by the amount of the rate. this way, the upper portion of the curve has slope 1.
			// we then add a y-offset to reset the threshold back to the original value
			auto modifiedThrehold = threshold * ratio;
			auto yOffset = modifiedThrehold - threshold;
			output = output * ratio - yOffset;
		}

		return output;
	}

};

#endif
