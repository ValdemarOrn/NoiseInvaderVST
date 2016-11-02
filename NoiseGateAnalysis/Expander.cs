using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NoiseGate
{
	public class Expander
	{
		private double prevInDb = -150.0;
		private double outputDb = -150.0;
		private double gainDb = 0.0;

		private double reductionDb;
		private double upperSlope;
		private double lowerSlope;
		private double thresholdDb;
		
		public Expander()
		{
			Update(-20, -100, 2.0);
		}

		public void Update(double thresholdDb, double reductionDb, double slope)
		{
			this.thresholdDb = thresholdDb;
            this.reductionDb = reductionDb;
			upperSlope = slope;
			lowerSlope = slope * 3;
		}

		public double GetOutput()
		{
			return gainDb;
		}

		public void Expand(double dbVal)
		{
			if (dbVal < reductionDb)
				dbVal = reductionDb;

			// 1. The two expansion curve form the upper and lower boundary of what the permitted "desired dB" value will be
			var upperDb = Compress(dbVal, thresholdDb, upperSlope, 4, true);
			var lowerDb = Compress(dbVal, thresholdDb + 6, lowerSlope, 4, true);

			// 2. The status quo, if neither curve is "hit", is to increase or reduce the desired dB by the 
			// change in input dB. This change is applied to whatever the current output dB currently is.
			var dbChange = dbVal - prevInDb;
			var desiredDb = outputDb + dbChange;

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
			var gainDiff = outputDb - dbVal;
			if (gainDiff < reductionDb)
				gainDiff = reductionDb;

			gainDb = gainDiff;
		}

		/// <summary>
		/// Given an input dB value, will compress or expand it according to the parameters specified
		/// </summary>
		private static double Compress(double x, double threshold, double ratio, double knee, bool expand)
		{
			// the assumed gain
			double output;
			var kneeLow = threshold - knee;
			var kneeHigh = threshold + knee;

			if (x <= kneeLow)
			{
				output = x;
			}
			else if (x >= kneeHigh)
			{
				var diff = x - threshold;
				var a = threshold + diff / ratio;
				output = a;
			}
			else // in knee, below threshold
			{
				// position on the interpolating line between the two parts of the compression curve
				var positionOnLine = (x - kneeLow) / (kneeHigh - kneeLow);
				var kDiff = knee * positionOnLine;
				var xa = kneeLow + kDiff;
				var ya = xa;
				var yb = threshold + kDiff / ratio;
				var slope = (yb - ya) / (knee);
				output = xa + slope * positionOnLine * knee;
			}

			// if doing expansion, adjust the slopes so that instead of compressing, the curve expands
			if (expand)
			{
				// to expand, we multiple the output by the amount of the rate. this way, the upper portion of the curve has slope 1.
				// we then add a y-offset to reset the threshold back to the original value
				var modifiedThrehold = threshold * ratio;
				var yOffset = modifiedThrehold - threshold;
				output = output * ratio - yOffset;
			}

			return output;
		}
	}
}
