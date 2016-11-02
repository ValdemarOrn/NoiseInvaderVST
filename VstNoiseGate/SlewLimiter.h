#pragma once

#include "AudioLib/Utils.h"

namespace NoiseInvader
{
	class SlewLimiter
	{
	private:
		double fs;
		double slewUp;
		double slewDown;
		double output;

	public:

		SlewLimiter(double fs)
		{
			this->fs = fs;
			this->slewUp = 1;
			this->slewDown = 1;
		}

		/// <summary>
		/// Computes the slew rates for fading 60 dB in the time specified
		/// </summary>
		void UpdateDb60(double slewUpMillis, double slewDownMillis)
		{
			auto upSamples = slewUpMillis / 1000.0 * fs;
			auto downSamples = slewDownMillis / 1000.0 * fs;
			this->slewUp = 60.0 / upSamples;
			this->slewDown = 60.0 / downSamples;
		}

		double Process(double value)
		{
			if (value > output)
			{
				if (value > output + slewUp)
					output = output + slewUp;
				else
					output = value;
			}
			else
			{
				if (value < output - slewDown)
					output = output - slewDown;
				else
					output = value;
			}

			return output;
		}
	};
}
