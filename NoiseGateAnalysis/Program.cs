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
		class Sma
		{
			private Queue<double> queue;
			private double sum;
			private double dbDecayPerSample;

			public double DbDecayPerSample { get { return dbDecayPerSample; } }

			public Sma(int samples)
			{
				this.queue = new Queue<double>();
				for (int i = 0; i < samples; i++)
				{
					queue.Enqueue(0.0);
				}
			}

			public double Update(double sample)
			{
				var takeAway = queue.Dequeue();
				queue.Enqueue(sample);
				sum -= takeAway;
				sum += sample;


				var sampleDb = Gain2Db(sample);
				var takeAwayDb = Gain2Db(takeAway);
				if (sampleDb < -150)
					sampleDb = -150;
				if (takeAwayDb < -150)
					takeAwayDb = -150;
				dbDecayPerSample = (sampleDb - takeAwayDb) / queue.Count;
				
                return sum / queue.Count;
			}
		}

		class Ema
		{
			private double value;
			private double alpha;

			public Ema(double alpha)
			{
				this.alpha = alpha;
			}

			public double Update(double sample)
			{
				value = sample * alpha + value * (1 - alpha);
				return value;
			}
		}

		class EmaLatch
		{
			private double value;
			private double alpha;
			private double latch;
			private double currentValue;

			public EmaLatch(double alpha, double latch)
			{
				this.alpha = alpha;
				this.latch = latch;
			}

			public double Update(bool input)
			{
				var sample = input ? 1.0 : -1.0;
				value = sample * alpha + value * (1 - alpha);

				if (value > latch)
					currentValue = 1.0;
				if (value < -latch)
					currentValue = -1.0;

				return currentValue;
			}
		}

		public static double ReductionDb = -100.0;
		public static double ThresholdDb = -20;
		public static double UpperSlope = 1.5;
		public static double LowerSlope = 9.0;
		public static double ReleaseMs = 20;
		public static double Volume = 1.0;

		private static double fs = 48000.0;
		private static double ts = 1.0 / fs;

		//private static double lpFc = 600.0;
		//private static double lpAlpha = (2 * Math.PI * ts * lpFc) / (2 * Math.PI * ts * lpFc + 1);
		private static Butterworth inputFilter = new Butterworth(fs);
		
		private static double emaFc = 200.0;
		private static double emaAlpha = (2 * Math.PI * ts * emaFc) / (2 * Math.PI * ts * emaFc + 1);

		private static double slowDbDecayPerSample = -60 / (3000 / 1000.0 * fs);
		private static double slowDecay = Db2Gain(slowDbDecayPerSample);

		private static double dbDecayPerSample = -60 / (ReleaseMs / 1000.0 * fs);
		private static double fastDecay = Db2Gain(dbDecayPerSample);

		private static double holdFc = 200.0;
		private static double holdAlpha = (2 * Math.PI * ts * holdFc) / (2 * Math.PI * ts * holdFc + 1);
		//private static Butterworth holdFilter = new Butterworth(fs);

		private static Sma sma = new Sma((int)(fs * 0.01));
		private static Ema ema = new Ema(emaAlpha);
		private static EmaLatch movementLatch = new EmaLatch(0.005, 0.2);

		public static double Db2Gain(double input)
		{
			return (double)Math.Pow(10, input / 20);
		}

		public static double Gain2Db(double input)
		{
			return (double)(20 * Math.Log10(input));
		}

		public double Compress(double valDb, double slope)
		{
			if (valDb > ThresholdDb)
				return valDb;

			var output = valDb * slope - ThresholdDb * (slope - 1);
			if (output < ReductionDb)
				return ReductionDb;

			return output;
		}

		private double lpValue;
		private double emaValue, smaValue, movementValue;
		private double combinedFiltered = 0.0;
		private int lastTriggerCounter = 0;
		private double hold = 0.0;
		private double h1, h2, h3, h4;
		private double holdFiltered = 0.1;
		private double decay = 0.9999;

		private void ProcessEnvelope(double val)
		{
			// 1. Rectify the input signal
			val = Math.Abs(val);

			// 2. Band pass filter to ~  100hz - 2Khz
			//lpValue = lpAlpha * val + (1 - lpAlpha) * lpValue;
			lpValue = inputFilter.Process(val);
			//hpSignal = hipassAlpha * lpSignal + (1 - hipassAlpha) * hpSignal

			// rectify the lpValue again, because the resonance in the filter can cause a tiny bit of ringing and cause the values to go negative again
			lpValue = Math.Abs(lpValue);

			var mainInput = lpValue;
			
			// 3. Compute the EMA and SMA of the band-filtered signal. Also compute the per-sample dB decay baed on the SMA
			emaValue = ema.Update(mainInput);
			smaValue = sma.Update(mainInput);

			// 4. use a latching low-pass classifier to determine if signal strength is generally increasing or decreasing.
			// This removes spike from the signal where the SMA may move in the opposite direction for a short period
			movementValue = movementLatch.Update(sma.DbDecayPerSample > 0);

			// 5. If the movement is going up, prefer the faster moving EMA signal if it's above the SMA
			// If the movement is going down, prefer the faster moving EMA signal if it's below the SMA
			// otherwise, use SMA
            if (movementValue > 0) // going up
				combinedFiltered = emaValue > smaValue ? emaValue : smaValue;
			else // going down
				combinedFiltered = emaValue < smaValue ? emaValue : smaValue;

			// 6. use a hold mechanism to store the peak
			if (combinedFiltered > hold)
			{
				hold = combinedFiltered;
				lastTriggerCounter = 0;
			}

			// 7. Choosing the decay speed
			// Under normal conditions, use the decay from the SMA, scaled by a fudge factor to make it slightly faster decaying.
			// The reason for this is so that we gently bump into the peaks of the signal once in a while.
			// If the hold mechanism hasn't been triggered for a specific timeout, then the current hold value is too high, and we need to rapidly decay downwards.
			// Use the fastDecay (based on the user- specified release value) as a slew limited value
			if (lastTriggerCounter > fs * 0.01)
				decay = fastDecay;
			else
				decay = Db2Gain(sma.DbDecayPerSample * 1.2); // 1.2 is fudge factor to make the follower decay slightly faster than actual signal, so we gently bump into the peaks

			// 7.5 Limit the decay speed in the general range of slowDecay...fastDecay, the slow decay is currently a fixed 3 seconds to -60dB value
			if (decay > slowDecay)
				decay = slowDecay;
			if (decay < fastDecay)
				decay = fastDecay;

			hold = hold * decay;

			// 8. Filter the resulting hold signal to retrieve a smooth envelope.
			// Currently using 4x 1 pole lowpass, should replace with a proper 4th order butterworth
			h1 = holdAlpha * hold + (1 - holdAlpha) * h1;
			h2 = holdAlpha * h1 + (1 - holdAlpha) * h2;
			h3 = holdAlpha * h2 + (1 - holdAlpha) * h3;
			h4 = holdAlpha * h3 + (1 - holdAlpha) * h4;

			holdFiltered = h4;
			//holdFiltered = holdFilter.Process(hold);
			lastTriggerCounter++;
		}

		public void Run()
		{
			inputFilter.Parameters[Butterworth.P_ORDER] = 6;
			inputFilter.Parameters[Butterworth.P_CUTOFF_HZ] = 1800;
			inputFilter.Update();

			//holdFilter.Parameters[Butterworth.P_ORDER] = 2;
			//holdFilter.Parameters[Butterworth.P_CUTOFF_HZ] = 100;
			//holdFilter.Update();
			
			/*var pm = new PlotModel();
			var xs = Enumerable.Range(0, 1000).Select(x => x / 1000.0).Select(x => -100 + x * 100).ToArray();
			var ys1 = xs.Select(x => Compress(x, UpperSlope)).ToArray();
			var ys2 = xs.Select(x => Compress(x, LowerSlope)).ToArray();

			pm.AddLine(xs, ys1);
			pm.AddLine(xs, ys2);
			pm.Show();*/

			var pm = new PlotModel();
			var signalValues = new List<double>();
			var emaValues = new List<double>();
			var smaValues = new List<double>();
			var movementValues = new List<double>();
			var combinedFilteredValues = new List<double>();

			var holdSignalValues = new List<double>();
			var filteredHoldSignalValues = new List<double>();

			var followerValues = new List<double>();

			var follower = new EnvelopeFollower(48000, 100);

			var rand = new Random();
			Func<double> random = () => 2 * rand.NextDouble() - 1;
			double decay = 1.0;
			for (int i = 0; i < 20000; i++)
			{
				var x = (Math.Sin(i / fs * 2 * Math.PI * 300) + random() * 0.4) * decay;
				decay *= 0.9998;
				if (i < 1000)
					x = random() * 0.001;
				else if (i > 12000)
				{
					x = random() * 0.001;
				}
				else
				{
					
				}

				follower.ProcessEnvelope(x);
				followerValues.Add(follower.GetOutput());

				ProcessEnvelope(x);

				signalValues.Add(x);
				emaValues.Add(emaValue);
				smaValues.Add(smaValue);
				movementValues.Add(movementValue);
				combinedFilteredValues.Add(combinedFiltered);
				holdSignalValues.Add(hold);
				filteredHoldSignalValues.Add(holdFiltered);

			}

			//pm.AddLine(signalValues).Title = "Signal";
			//pm.AddLine(emaValues.Select(Gain2Db)).Title = "emaValues";
			//pm.AddLine(smaValues.Select(Gain2Db)).Title = "smaValues";
			//pm.AddLine(movementValues.Select(x => x > 0 ? 1.0 : 0.0)).Title = "movementValues";
			//pm.AddLine(combinedFilteredValues.Select(Gain2Db)).Title = "Combined Filtered";
			//pm.AddLine(holdSignalValues.Select(Gain2Db)).Title = "Hold";
			pm.AddLine(filteredHoldSignalValues.Select(Gain2Db)).Title = "Filtered Hold";
			pm.AddLine(followerValues.Select(Gain2Db)).Title = "followerValues";

			pm.Show();
		}

		[STAThread]
		static void Main(string[] args)
		{
			new Program().Run();
		}

	}
}
