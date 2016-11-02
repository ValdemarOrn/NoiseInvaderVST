#pragma once

#include <iostream>
#include <cmath>

#include "AudioLib/Sse.h"
#include "Expander.h"
#include "EnvelopeFollower.h"

using namespace AudioLib;

namespace NoiseInvader
{
	class NoiseGateKernel2
	{
	private:

		float fs;

		EnvelopeFollower envelopeFollower;
		Expander expander;
			
	public:

		// Gain Settings
		float DetectorGain;
		
		// Noise Gate Settings
		double ReductionDb;
		double ThresholdDb;
		double Slope;
		double ReleaseMs;

		NoiseGateKernel2(int fs)
			: envelopeFollower(fs, 100)
			, expander()
		{
			this->fs = fs;
			
			DetectorGain = 1.0f;
			ReductionDb = -150;
			ThresholdDb = -20;
			Slope = 3;
			ReleaseMs = 100;
			UpdateAll();
		}

		inline ~NoiseGateKernel2()
		{

		}

		inline void UpdateAll()
		{
			expander.Update(ThresholdDb, ReductionDb, Slope);
			envelopeFollower.SetRelease(ReleaseMs);
		}

		inline void Process(float* input, float* detectorInput, float* output, int len)
		{
			Sse::PreventDernormals();

			for (size_t i = 0; i < len; i++)
			{
				auto x = detectorInput[i] * DetectorGain;
				envelopeFollower.ProcessEnvelope(x);
				auto env = envelopeFollower.GetOutput();

				expander.Expand(Utils::Gain2DB(env));
				auto gain = expander.GetOutput();

				output[i] = input[i] * gain;
			}
		}

	};
}

