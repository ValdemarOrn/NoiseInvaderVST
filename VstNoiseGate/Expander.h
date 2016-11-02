#pragma once

#include "AudioLib/Utils.h"

namespace NoiseInvader
{
	class Expander
	{
	private:

		double prevInDb = -150.0;
		double outputDb = -150.0;
		double gain = 1.0;

		double reductionDb;
		double upperSlope;
		double lowerSlope;
		double thresholdDb;

	public:
		Expander()
		{
			Update(-20, -100, 2.0);
		}

		~Expander()
		{

		}

		void Update(double thresholdDb, double reductionDb, double slope)
		{
			this->thresholdDb = thresholdDb;
			this->reductionDb = reductionDb;
			upperSlope = slope;
			lowerSlope = slope * 3;
		}

		inline double GetOutput()
		{
			return gain;
		}

		void Expand(double dbVal)
		{
			if (dbVal < reductionDb)
				dbVal = reductionDb;

			// 1. The two expansion curve form the upper and lower boundary of what the permitted "desired dB" value will be
			auto upperDb = Compress(dbVal, thresholdDb, upperSlope, 0, true);
			auto lowerDb = Compress(dbVal, thresholdDb, lowerSlope, 0, true);

			// 2. The status quo, if neither curve is "hit", is to increase or reduce the desired dB by the 
			// change in input dB. This change is applied to whatever the current output dB currently is.
			auto dbChange = dbVal - prevInDb;
			auto desiredDb = outputDb + dbChange;

			// 3. apply the upper and lower curves as limits to the desired dB
			if (desiredDb < lowerDb)
			{
				desiredDb = lowerDb;
			}
			else if (desiredDb > upperDb)
			{
				desiredDb = upperDb;
			}

			outputDb = desiredDb;
			prevInDb = dbVal;

			// 4. do not let the effective gain go below the reductiondB value
			auto gainDiff = outputDb - dbVal;
			if (gainDiff < reductionDb)
				gainDiff = reductionDb;

			gain = AudioLib::Utils::DB2gain(gainDiff);
		}

	private:

		/// <summary>
		/// Given an input dB value, will compress or expand it according to the parameters specified
		/// </summary>
		static double Compress(double x, double threshold, double ratio, double knee, bool expand)
		{
			// the assumed gain
			double output;
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
}
