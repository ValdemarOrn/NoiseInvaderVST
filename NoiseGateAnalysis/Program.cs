using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using AudioLib;
using AudioLib.Modules;
using AudioLib.TF;
using LowProfile.Visuals;
using OxyPlot;
using OxyPlot.Axes;

namespace NoiseGate
{
	class Program
	{
		public static double ReductionDb = -150.0;
		public static double ThresholdDb = -20;
		public static double UpperSlope = 3;
		public static double LowerSlope = 5;
		public static double ReleaseMs = 20;
		public static double Volume = 1.0;

		private static double fs = 48000.0;
		private static double ts = 1.0 / fs;

		public double HardExpand(double valDb, double thresholdDb, double slope)
		{
			if (valDb > thresholdDb)
				return valDb;

			var output = valDb * slope - thresholdDb * (slope - 1);
			return output;
		}

		static double Compress(double x, double threshold, double ratio, double knee, bool expand)
		{
			// the assumed gain
			var output = x;
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

		private double prevInDb = -150.0;
		private double outputDb = -150.0;
		private double gain = 1.0;

		public void Expand(double dbVal)
		{
			if (dbVal < ReductionDb)
				dbVal = ReductionDb;

			var upperDb = Compress(dbVal, ThresholdDb, UpperSlope, 0, true);
			var lowerDb = Compress(dbVal, ThresholdDb, LowerSlope, 0, true);

			var dbChange = dbVal - prevInDb;
			var desiredDb = outputDb + dbChange;

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

			var gainDiff = outputDb - dbVal;
			if (gainDiff < ReductionDb)
				gainDiff = ReductionDb;
            gain = Utils.Db2Gain(gainDiff);
		}

		public void Run()
		{
			/*var pm = new PlotModel();
			var xs = Enumerable.Range(0, 1000).Select(x => x / 1000.0).Select(x => -100 + x * 100).ToArray();
			var ys1 = xs.Select(x => Compress(x, ThresholdDb, UpperSlope, 3, true)).ToArray();
			var ys2 = xs.Select(x => Compress(x, ThresholdDb + 6, LowerSlope, 3, true)).ToArray();

			pm.AddLine(xs, ys1);
			pm.AddLine(xs, ys2);
			pm.Show();
			*/
			var pm = new PlotModel();
			pm.Axes.Add(new LinearAxis { Position = AxisPosition.Left, Key = "L" });
			pm.Axes.Add(new LinearAxis { Position = AxisPosition.Right, Key = "R" });
			
			var follower = new EnvelopeFollower(48000, 150);
			var signalValues = new List<double>();
			var followerValues = new List<double>();
			var gainValues = new List<double>();
			var dbOutValues = new List<double>();

			var rand = new Random();
			Func<double> random = () => 2 * rand.NextDouble() - 1;
			double decay = 1.0;
			for (int i = 0; i < 20000; i++)
			{
				var x = (Math.Sin(i / fs * 2 * Math.PI * 100) + random() * 0.4) * (1 + Math.Sin(i / 300.0) * 0.8) * 0.3;
				decay *= 0.9998;
				if (i < 1000)
					x = random() * 0.001;
				else if (i > 12000)
				{
					x = random() * 0.001;
				}

				signalValues.Add(x);
				follower.ProcessEnvelope(x);
				var env = follower.GetOutput();

				Expand(Utils.Gain2Db(env));
				gainValues.Add(gain);
				dbOutValues.Add(outputDb);

                followerValues.Add(env);
			}

			var signalLine = pm.AddLine(signalValues);
			signalLine.Title = "Signal";
			signalLine.YAxisKey = "R";
			pm.AddLine(followerValues.Select(Utils.Gain2Db)).Title = "followerValues";
			pm.AddLine(gainValues.Select(Utils.Gain2Db)).Title = "gainValues";

			pm.Show();
		}

		[STAThread]
		static void Main(string[] args)
		{
			new Program().Run();
		}

	}
}
