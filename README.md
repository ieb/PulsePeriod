# Measuring pulse periods.

This project was created to verify that measuring micros between pulses is a reasonable way to measure engine RPM.  TLDR; It is, but has slightly more overhead than direct use of timers.

# Methods

1 Measure the micros between 10 rising edges, using a uint32_t for the micros.
2 Setup a Free running counter and measure the count of the counter between a set number of pulses, adjusting the 16bit frequency of the counter to accommodate a range of frequencies. Advantage, smaller ISR with no function calls. Disadvantage, counter has to be reset on each period measured, and there are overflows to handle. Also not portable.
3 Use the 16bit counter to interrupt on edges and adjust the frequency of the counter to avoid overlflows. Advantages, smaller ISR again, but only 1 timer of this type is available 328p. Also not portable.

# Results

micros is good upto 31KHz and perhaps further. Errors relative to the cpu clock are not measurable although the CPU clock has errors relative to real time. This impacts all methods. 

Method 2 is not compltely reliable due to autoscaling and the errors show that reseting the counter on each measurement introduces some jitter in errors.

Method 3 is not really usable, or at least I didnt get it to work.

# Conclusion

For engines not expected to rev over 6K RPM and with < 300 sensor teeth on a flywheel, the micros method is accurate and stable. Coupled with an external cpu crystal with low drift it should be accurate. Should also work fine upto 18K rpm with < 100 sensor teeth.