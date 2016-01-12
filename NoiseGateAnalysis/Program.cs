using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using AudioLib;
using AudioLib.Modules;
using LowProfile.Visuals;
using OxyPlot;
using OxyPlot.Axes;

namespace NoiseGate
{
	class Program
	{
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

		static double fs = 48000.0;

		[STAThread]
		static void Main(string[] args)
		{
			var SignalFloor = -140;

			/*var pm2 = new PlotModel();
			var px = Enumerable.Range(0, 1000).Select(x => x / 1000.0).Select(x => -150 + x * 150);
			pm2.Axes.Add(new LinearAxis { Position = AxisPosition.Left, Minimum = -150, Maximum = 0 });
			pm2.AddLine(px, x => Math.Max(Compress(x, -40, 1.0, 0.001, true), SignalFloor), x => x).LineStyle = LineStyle.Dot;
			pm2.AddLine(px, x => Math.Max(Compress(x, -60, 0.2, 0.001, true), SignalFloor), x => x);
			pm2.AddLine(px, x => Math.Max(Compress(x, -90, 2.0, 0.001, true), SignalFloor), x => x);
			OxyPlot.Wpf.PngExporter.Export(pm2, @"c:\chart2.png", 1600, 1000, OxyColors.White);
			pm2.Show();*/

			//var wav = AudioLib.WaveFiles.ReadWaveFile(@"C:\Users\Valdemar\Desktop\wogain2.wav")[0];
            //var xs = Enumerable.Range(0, wav.Length).Select(x => x / fs).ToArray();
            //var ys = wav.ToArray();

            var xs = Enumerable.Range(0, 96000).Select(x => x / fs).ToArray();
		    var ys = new double[xs.Length];
			var peaks = new double[xs.Length];
			var envLin = new double[xs.Length];
			var effectiveCurveDb = new double[xs.Length];
			var filteredEnvDb = new double[xs.Length];
            var gCurve = new double[xs.Length];
            var outputs = new double[xs.Length];

			
			var thresholdOpen = -3.0;
		    var thresholdClose = -3.0;
		    var ratioOpen = 1.0000001;
			var ratioClose = 4;
			PeakDetector detector = new PeakDetector(fs);
			double envelopeDbFilterTemp = SignalFloor;
			double envelopeDbValue = SignalFloor;
		    double aPrevDb = SignalFloor;

            var fc = 100.0;
			var alpha = (2 * Math.PI * fc / fs) / (2 * Math.PI * fc / fs + 1);

			var attackMs = 1;
			var releaseMs = 1;
			var attackSlew = (100 / fs) / (attackMs * 0.001); // 100 dB movement in a given time period
			var releaseSlew = (100 / fs) / (releaseMs * 0.001);

			for (int i = 0; i < ys.Length; i++)
			{
				var portion = i / (double)ys.Length;

				var g = 1.0;
			    //g = Math.Abs(Math.Sin(portion * Math.PI * 2));
			    //if (portion > 0.5 && portion < 0.55)
			    //    g = 0;
				//g = (int)(portion * 3) % 2;
				g = 1-portion;
			    //g += Math.Sin(portion * 2 * Math.PI * 5) * 0.2;
			   // if (portion > 0.8)
			    //    g = 0.0001;
				
                ys[i] = g * Math.Sin(i / 480.0 * 2 * Math.PI);
			}

			for (int i = 0; i < ys.Length; i++)
			{
				var val = Math.Abs(ys[i]);

				// ------ Peak tracking --------
				var peakVal = detector.ProcessPeaks(val);
				peaks[i] = peakVal;

				// ------ Env Shaping --------

				var peakValDb = Utils.Gain2DB(peakVal);
                if (peakValDb < SignalFloor)
                    peakValDb = SignalFloor;

               /* // dynamically tweak the smoothing cutoff based on if the env. is above or below threshold
			    if (envelopeDbValue > thresholdOpen && fc != 10.0)
			    {
                    fc = 10.0;
                    alpha = (2 * Math.PI * fc / fs) / (2 * Math.PI * fc / fs + 1);
                }
                else if (envelopeDbValue < thresholdClose && fc != 100.0)
                {
                    fc = 100.0;
                    alpha = (2 * Math.PI * fc / fs) / (2 * Math.PI * fc / fs + 1);
                }*/

				envelopeDbFilterTemp = envelopeDbFilterTemp * (1 - alpha) + peakValDb * alpha;
				envelopeDbValue = envelopeDbValue * (1 - alpha) + envelopeDbFilterTemp * alpha;
				filteredEnvDb[i] = envelopeDbValue;

                // The two expansion curves form upper and lower limits on the signal
                var aDbUpperLim = Compress(envelopeDbValue, thresholdClose, ratioClose - 0.999, 0.005, true); // why the ratio - 0.999 ?? Read the section below on reference value!
				var aDbLowerLim = Compress(envelopeDbValue, thresholdOpen, ratioOpen - 0.999, 0.005, true); // if we subtract -1 it goes all bananas
				// If you use a higher ratio for the Close curve, then the curves can intersect. Prefer the lower of the two values
				if (aDbLowerLim > aDbUpperLim) aDbLowerLim = aDbUpperLim;
				// Limit the values to the signal floor
				if (aDbUpperLim < SignalFloor) aDbUpperLim = SignalFloor;
                if (aDbLowerLim < SignalFloor) aDbLowerLim = SignalFloor;
                var aDb = 0.0;

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

                var cValue = Utils.DB2gain(aDb);
				var cEnvValue = Utils.DB2gain(envelopeDbValue);
                effectiveCurveDb[i] = aDb;
				envLin[i] = cEnvValue;

				// this is the most important bit. The effective gain is computed gain is the envelope value / closeThreshold.
				// Now, since the closeThreshold > openThreshold, the gain is too high on the attack, but I haven't been able to
				// figure out a way to do it correctly on both attack and release. The dual theshold makes it hard to compute (impossible?)
				// I use the closeThreshold as a substitute in both cases, as we limit the g <= 1, but using the OpenTheshold would introduce
				// a gentle slope when the envelope is decaying between the two thresholds. The user can easily supplement for this extra gain by
				// adjusting the attack or open threshold to get the desired sound, though.
				// Note: using the static threshold as reference, rather than the envelope signal itself, means that there is always some expansion going on.
				// But if we use 0 < ratio < 1, we can achieve expansion from unity to 2! The effective expansion is one less than the expansion curve states.
				var g = cValue / Utils.DB2gain(thresholdClose);
			    if (g > 1) g = 1;
			    gCurve[i] = Utils.Gain2DB(g);

                outputs[i] = ys[i] * g;
			}

			var pm = new PlotModel();
			pm.Axes.Add(new LinearAxis { Position = AxisPosition.Left, Key = "main" });
			pm.Axes.Add(new LinearAxis { Position = AxisPosition.Right, Key = "db" });

			var l1 = pm.AddLine(ys);
			l1.Title = "Input";
			l1.YAxisKey = "main";

			/*var ll = pm.AddLine(peaks.Select(x => Utils.Gain2DB(x)));
			ll.Title = "Peaks";
			ll.YAxisKey = "db";*/
			
			var l2 = pm.AddLine(filteredEnvDb);
			l2.Title = "Filtered Env Log";
			l2.YAxisKey = "db";

			/*var lo = pm.AddLine(filteredEnvDb.Select(x => thresholdOpen));
			lo.LineStyle = LineStyle.Dash;
			lo.StrokeThickness = 0.5;
			lo.Title = "Threshold Open";
			lo.YAxisKey = "db";

			var lc = pm.AddLine(filteredEnvDb.Select(x => thresholdClose));
			lc.LineStyle = LineStyle.Dash;
			lc.StrokeThickness = 0.5;
			lc.Title = "Threshold Close";
			lc.YAxisKey = "db";*/

			var l3 = pm.AddLine(effectiveCurveDb);
			l3.Title = "Effective Curve Log";
			l3.YAxisKey = "db";

            var l4 = pm.AddLine(gCurve);
            l4.Title = "G Curve";
            l4.YAxisKey = "db";

            //pm.AddLine(envLin).Title = "Env Lin";

			pm.AddLine(outputs).Title = "Output";
			
			pm.Show();
			OxyPlot.Wpf.PngExporter.Export(pm, @"c:\chart.png", 1600, 1000, OxyColors.White);
		}
	}

	class PeakDetector
	{
		private double fs;

		private double decay = 0.995;
		private int PEAK_COUNT;
		private Tuple<int, double>[] peakStorage;
		private double prevValue;
		private int timeIndex;
		private int peakReadIndex;
		private int peakWriteIndex;

		private double currentValue;

		public PeakDetector(double fs)
		{
			this.fs = fs;
			PEAK_COUNT = (int)(10.0 / 1000 * fs);
			peakStorage = new Tuple<int, double>[PEAK_COUNT];
		}

		public double ProcessPeaks(double val)
		{
			if (val < prevValue) // we just saw a peak, store it
			{
				peakStorage[peakWriteIndex] = Tuple.Create(timeIndex, prevValue);
				peakWriteIndex = (peakWriteIndex + 1) % PEAK_COUNT;
			}

			// find peak
			Tuple<int, double> maxPeak = null;
			int readIdx = peakReadIndex;
			int minTimeIndex = timeIndex - PEAK_COUNT;
			while (readIdx != peakWriteIndex)
			{
				var p = peakStorage[readIdx];
				if (p.Item1 < minTimeIndex)
				{
					// this is old data, move read header
					peakReadIndex = (peakReadIndex + 1) % PEAK_COUNT;
				}
				else
				{
					if (maxPeak == null || p.Item2 > maxPeak.Item2)
						maxPeak = p;
				}

				readIdx = (readIdx + 1) % PEAK_COUNT;
			}

			var fallbackValue = currentValue * decay;
			if (fallbackValue < val)
				fallbackValue = val;

			if (maxPeak != null && maxPeak.Item2 > fallbackValue)
			{
				currentValue = maxPeak.Item2;
			}
			else
			{
				currentValue = fallbackValue;
			}

			prevValue = val;
			timeIndex++;

			return currentValue;
		}
	}
}
