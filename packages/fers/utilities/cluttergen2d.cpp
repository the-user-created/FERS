// SPDX-License-Identifier: GPL-2.0-only
//
// Copyright (c) 2008 Marc Brooker and Michael Inggs
// Copyright (c) 2008-present FERS Contributors (see AUTHORS.md).
//
// See the GNU GPLv2 LICENSE file in the FERS project root for more information.

/// Two dimensional clutter generator, this is an extremely over-simplified program, but it does the job

#include <fstream>   // for operator<<, basic_ostream, char_traits, basic_of...
#include <iostream>  // for cin, cout
#include <random>    // for uniform_real_distribution, normal_distribution
#include <string>    // for operator>>, string

using namespace std;

int main()
{
	int samples;
	cout << "Number of clutter samples: ";
	cin >> samples;
	double start_range_x;
	cout << "Start range x: ";
	cin >> start_range_x;
	double range_x;
	cout << "Range x: ";
	cin >> range_x;

	double start_range_y;
	cout << "Start range y: ";
	cin >> start_range_y;
	double range_y;
	cout << "Range y: ";
	cin >> range_y;

	double rcs;
	cout << "RCS: ";
	cin >> rcs;
	double spread;
	cout << "Stdev of spreading: ";
	cin >> spread;
	double time = 0;
	if (spread != 0)
	{
		cout << "Simulation end time: ";
		cin >> time;
	}
	string filename;
	cout << "Filename: ";
	cin >> filename;
	// Open the file
	ofstream fo(filename.c_str());

	// Create the rng
	random_device rd;
	mt19937 rng(rd());

	uniform_real_distribution ud_x(start_range_x, start_range_x + range_x);
	uniform_real_distribution ud_y(start_range_y, start_range_y + range_y);
	normal_distribution<> nd(0, spread);

	fo << "<incblock>";
	for (int i = 0; i < samples; i++)
	{
		fo << "<platform name=\"clutter\">\n";
		fo << "<motionpath interpolation=\"cubic\">\n";
		double pos_x = ud_x(rng);
		double pos_y = ud_y(rng);
		fo << "<positionwaypoint>\n<x>" << pos_x << "</x>\n<y>" << pos_y <<
			"</y>\n<altitude>0</altitude>\n<time>0</time>\n</positionwaypoint>\n";
		fo << "<positionwaypoint>\n<x>" << pos_x + time * nd(rng) << "</x>\n<y>" << pos_y + time * nd(rng) <<
			"</y>\n<altitude>0</altitude>\n<time>" << time << "</time>\n</positionwaypoint>\n";
		fo << "</motionpath>\n";
		fo <<
			"<fixedrotation><startazimuth>0.0</startazimuth><startelevation>0.0</startelevation><azimuthrate>0</azimuthrate><elevationrate>0</elevationrate></fixedrotation>\n";
		fo << "<target name=\"wings\">\n<rcs type=\"isotropic\">\n<value>";
		fo << rcs;
		fo << "</value>\n</rcs>\n</target>\n</platform>\n\n";
	}
	fo << "</incblock>";

	return 0;
}
