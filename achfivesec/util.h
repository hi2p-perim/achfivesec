#pragma once
#ifndef ACHFIVESEC_UTIL_H
#define ACHFIVESEC_UTIL_H

#include "common.h"

class Util
{
private:

	Util();
	FW_DISABLE_COPY_AND_MOVE(Util);

public:

	static double BPM()							{ return 164; }
	static double RPM()							{ return 8; }
	static double MilliToBeats(double milli)	{ return milli * BPM() / 60000.0; }
	static double BeatsToMilli(double beats)	{ return beats * 60000.0 / BPM(); }
	static double RowToMilli(double row)		{ return BeatsToMilli(row / RPM()); }
	static double MilliToRow(double milli)		{ return MilliToBeats(milli) * RPM(); }

};

#endif // ACHFIVESEC_UTIL_H