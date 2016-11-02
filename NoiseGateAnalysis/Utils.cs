using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NoiseGate
{
	public static class Utils
	{
		public static double Db2Gain(double input)
		{
			return (double)Math.Pow(10, input / 20);
		}

		public static double Gain2Db(double input)
		{
			return (double)(20 * Math.Log10(input));
		}
	}
}
