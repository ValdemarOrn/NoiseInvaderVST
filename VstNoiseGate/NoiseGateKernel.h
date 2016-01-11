#ifndef _NoiseGateKernel
#define _NoiseGateKernel

#include <iostream>
#include <cmath>
#include "AudioLib/MathDefs.h"
#include "AudioLib/Biquad.h"
#include "AudioLib/Utils.h"
#include "AudioLib/Sse.h"
#include "PeakDetector.h"

using namespace AudioLib;

class NoiseGateKernel
{
private:

	float fs;
		
	Biquad lowpass;
	Biquad highpass;
	PeakDetector detector;

	// temp values
	float envelopeDbFilterTemp;
	float envelopeDbValue;
	float aPrevDb;
	
	// envelope settings - internal
	float attackSlew;
	float releaseSlew;
	// These values are actually fixed, they set the parameters of the smoothing envelope filter
	float envelopeCutoffHz;
	float envelopeAlpha;

public:
	
	bool Enabled;

	// Gain Settings
	float InputGain;
	float OutputGain;

	// Envelope Settings
	float AttackMs;
	float ReleaseMs;

	// expander settings
	float SignalFloor;
	float RatioOpen;
	float RatioClose;
	float ThresholdOpenDb;
	float ThresholdCloseDb;
	float KneeDb;

	// signal shaping
	float LowpassHz;
	float HighpassHz;

	NoiseGateKernel(int fs)
		: detector(fs)
	{
		this->fs = fs;
		SignalFloor = -150.0f;

		// The smoothing is fixed at 100Hz cutoff
		envelopeCutoffHz = 100.0f; 
		envelopeAlpha = 0;
		float k = 2 * M_PI * envelopeCutoffHz / fs;
		envelopeAlpha = (k) / (k + 1);

		envelopeDbFilterTemp = SignalFloor;
		envelopeDbValue = SignalFloor;
		aPrevDb = SignalFloor;
		attackSlew = 0;
		releaseSlew = 0;

		InputGain = 1.0f;
		OutputGain = 1.0f;

		AttackMs = 5.f;
		ReleaseMs = 50.f;

		RatioOpen = 3.0f;
		RatioClose = 3.0f;
		ThresholdOpenDb = -20.f;
		ThresholdCloseDb = -10.f;
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
		Sse::PreventDernormals();

		if (!Enabled)
		{
			for (size_t i = 0; i < len; i++)
				output[i] = input[i];

			return;
		}

		float g = 1.0f;
		float referenceValue = Utils::DB2gain(ThresholdCloseDb);

		for (size_t i = 0; i < len; i++)
		{
			float x = lowpass.Process(input[i] * InputGain);
			x = highpass.Process(x);
			float absVal = abs(x);

			// ------ Peak tracking --------
			auto peakVal = detector.ProcessPeaks(x);

			// ------ Env Shaping --------

			auto peakValDb = Utils::Gain2DB(peakVal);
			if (peakValDb < SignalFloor)
				peakValDb = SignalFloor;

			// dynamically tweak the smoothing cutoff based on if the env. is above or below threshold
			// https://en.wikipedia.org/wiki/Low-pass_filter  - for alpha computation
			if (envelopeDbValue > ThresholdOpenDb && envelopeCutoffHz != 10.0)
			{
				envelopeCutoffHz = 10.0;
				
				float k = 2 * M_PI * envelopeCutoffHz / fs;
				envelopeAlpha = (k) / (k + 1);
			}
			else if (envelopeDbValue < ThresholdCloseDb && envelopeCutoffHz != 100.0)
			{
				envelopeCutoffHz = 100.0;
				
				float k = 2 * M_PI * envelopeCutoffHz / fs;
				envelopeAlpha = (k) / (k + 1);
			}

			envelopeDbFilterTemp = envelopeDbFilterTemp * (1 - envelopeAlpha) + peakValDb * envelopeAlpha;
			envelopeDbValue = envelopeDbValue * (1 - envelopeAlpha) + envelopeDbFilterTemp * envelopeAlpha;

			// The two expansion curves form upper and lower limits on the signal
			auto aDbUpperLim = Compress(envelopeDbValue, ThresholdCloseDb, RatioClose, KneeDb, true);
			auto aDbLowerLim = Compress(envelopeDbValue, ThresholdOpenDb, RatioOpen, KneeDb, true);
			// If you use a higher ratio for the Close curve, then the curves can intersect. Prefer the lower of the two values
			if (aDbLowerLim > aDbUpperLim) aDbLowerLim = aDbUpperLim;
			// Limit the values to the signal floor
			if (aDbUpperLim < SignalFloor) aDbUpperLim = SignalFloor;
			if (aDbLowerLim < SignalFloor) aDbLowerLim = SignalFloor;
			float aDb = 0.0f;

			// compare the current gain to the upper and lower limits, and clamp the new value between those.
			// slew limit the change in gain with the attack and release params.
			if (aPrevDb < aDbLowerLim)
			{
				aDb = aPrevDb + attackSlew;
				if (aDb > aDbLowerLim)
					aDb = aDbLowerLim;
			}
			else if (aPrevDb > aDbUpperLim)
			{
				aDb = aPrevDb - releaseSlew;
				if (aDb < aDbUpperLim)
					aDb = aDbUpperLim;
			}
			else
			{
				// If we do not "bump into" either the upper or the lower limit, meaning we are somewhere in the
				// middle of the histeresis region, then leave the current gain unchanged.
				aDb = aPrevDb;
			}

			aPrevDb = aDb;

			auto cValue = Utils::DB2gain(aDb);
			auto cEnvValue = Utils::DB2gain(envelopeDbValue);

			// this is the most important bit. The effective gain is computed gain is the envelope value / closeThreshold.
			// Now, since the closeThreshold > openThreshold, the gain is too high on the attack, but I haven't been able to
			// figure out a way to do it correctly on both attack and release. The dual theshold makes it hard to compute (impossible?)
			// I use the closeThreshold as a substitute in both cases, as we limit the g <= 1, but using the OpenTheshold would introduce
			// a gentle slope when the envelope is decaying between the two thresholds. The user can easily supplement for this extra gain by
			// adjusting the attack or open threshold to get the desired sound, though.
			auto g = cValue / referenceValue;
			if (g > 1) g = 1;
			output[i] = input[i] * g * OutputGain;
		}
	}

	inline void UpdateAll()
	{
		attackSlew = (100 / fs) / (AttackMs * 0.001); // 100 dB movement in a given time period
		releaseSlew = (100 / fs) / (ReleaseMs * 0.001);
		
		lowpass.Frequency = LowpassHz;
		highpass.Frequency = HighpassHz;
		lowpass.Update();
		highpass.Update();
	}

private:

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
