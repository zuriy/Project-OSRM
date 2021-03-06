/*
 open source routing machine
 Copyright (C) Dennis Luxen, others 2010

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU AFFERO General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 or see http://www.gnu.org/licenses/agpl.txt.
 */

#include "DescriptionFactory.h"

DescriptionFactory::DescriptionFactory() { }

DescriptionFactory::~DescriptionFactory() { }

double DescriptionFactory::GetAzimuth(const _Coordinate& A, const _Coordinate& B) const {
    double lonDiff = (A.lon-B.lon)/100000.;
    double angle = atan2(sin(lonDiff)*cos(B.lat/100000.),
            cos(A.lat/100000.)*sin(B.lat/100000.)-sin(A.lat/100000.)*cos(B.lat/100000.)*cos(lonDiff));
    angle*=180/M_PI;
    while(angle < 0)
        angle += 360;

    return angle;
}


void DescriptionFactory::SetStartSegment(const PhantomNode & _startPhantom) {
    startPhantom = _startPhantom;
    AppendSegment(_startPhantom.location, _PathData(0, _startPhantom.nodeBasedEdgeNameID, 10, _startPhantom.weight1));
}

void DescriptionFactory::SetEndSegment(const PhantomNode & _targetPhantom) {
    targetPhantom = _targetPhantom;
    pathDescription.push_back(SegmentInformation(_targetPhantom.location, _targetPhantom.nodeBasedEdgeNameID, 0, _targetPhantom.weight1, 0, true) );
}

void DescriptionFactory::AppendSegment(const _Coordinate & coordinate, const _PathData & data ) {
    pathDescription.push_back(SegmentInformation(coordinate, data.nameID, 0, data.durationOfSegment, data.turnInstruction) );
}

void DescriptionFactory::AppendEncodedPolylineString(std::string & output, bool isEncoded) {
    if(isEncoded)
        pc.printEncodedString(pathDescription, output);
    else
        pc.printUnencodedString(pathDescription, output);
}

void DescriptionFactory::AppendEncodedPolylineString(std::string &output) {
    pc.printEncodedString(pathDescription, output);
}

void DescriptionFactory::AppendUnencodedPolylineString(std::string &output) {
    pc.printUnencodedString(pathDescription, output);
}

unsigned DescriptionFactory::Run(const unsigned zoomLevel) {
    if(0 == pathDescription.size())
        return 0;

    unsigned entireLength = 0;
    /** starts at index 1 */
    pathDescription[0].length = 0;
    for(unsigned i = 1; i < pathDescription.size(); ++i) {
        pathDescription[i].length = ApproximateDistance(pathDescription[i-1].location, pathDescription[i].location);
    }

    unsigned lengthOfSegment = 0;
    unsigned durationOfSegment = 0;
    unsigned indexOfSegmentBegin = 0;

    for(unsigned i = 1; i < pathDescription.size(); ++i) {
        entireLength += pathDescription[i].length;
        lengthOfSegment += pathDescription[i].length;
        durationOfSegment += pathDescription[i].duration;
        pathDescription[indexOfSegmentBegin].length = lengthOfSegment;
        pathDescription[indexOfSegmentBegin].duration = durationOfSegment;
        if(pathDescription[i].turnInstruction != 0) {
            //INFO("Turn after " << lengthOfSegment << "m into way with name id " << segment.nameID);
            assert(pathDescription[i].necessary);
            lengthOfSegment = 0;
            durationOfSegment = 0;
            indexOfSegmentBegin = i;
        }
    }
    if(pathDescription[0].length == 0){
        pathDescription[0].turnInstruction = 14;
    }

    //Generalize poly line
    dp.Run(pathDescription, zoomLevel);

    //fix what needs to be fixed else
    for(unsigned i = 0; i < pathDescription.size()-1 && pathDescription.size() >= 2; ++i){
        if(pathDescription[i].necessary) {
            int angle = 100*GetAzimuth(pathDescription[i].location, pathDescription[i+1].location);
            pathDescription[i].bearing = angle/100.;
        }
    }

    return entireLength;
}
