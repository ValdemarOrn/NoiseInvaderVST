using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NoiseGate
{
	public class SlewLimiter
	{
		private double fs;
		private double slewUp;
		private double slewDown;
		private double output;

		public SlewLimiter(double fs)
		{
			this.fs = fs;
			this.slewUp = 1;
			this.slewDown = 1;
		}

		/// <summary>
		/// Computes the slew rates for fading 60 dB in the time specified
		/// </summary>
		public void UpdateDb60(double slewUpMillis, double slewDownMillis)
		{
			var upSamples = slewUpMillis / 1000.0 * fs;
			var downSamples = slewDownMillis / 1000.0 * fs;
			this.slewUp = 60 / upSamples;
			this.slewDown = 60 / downSamples;
		}

		public double Process(double value)
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
	}
}
