/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/globals.h"
#include "common/tag.h"
#include "common/Format.h"
#include "common/string.h"
#include "common/limits.h"
#include "alarmMgr/alarmMgr.h"

#include "ngsi/Scope.h"
#include "parse/forbiddenChars.h"

using namespace orion;



/* ****************************************************************************
*
* Scope::Scope -
*/
Scope::Scope()
{
  type     = "";
  value    = "";
  oper     = "";
  areaType = orion::NoArea;

  georel.maxDistance = -1;
  georel.minDistance = -1;
}



/* ****************************************************************************
*
* Scope::Scope -
*/
Scope::Scope(const std::string& _type, const std::string& _value, const std::string& _oper)
{
  type     = _type;
  value    = _value;
  oper     = _oper;
  areaType = orion::NoArea;

  georel.maxDistance = -1;
  georel.minDistance = -1;
}



/* ****************************************************************************
*
* pointVectorRelease - 
*/
static void pointVectorRelease(const std::vector<orion::Point*>& pointV)
{
  for (unsigned int ix = 0; ix < pointV.size(); ++ix)
  {
    delete(pointV[ix]);
  }
}



/* ****************************************************************************
*
* Scope::fill - 
*/
int Scope::fill
(
  const std::string&  apiVersion,
  const std::string&  geometryString,
  const std::string&  coordsString,
  const std::string&  georelString,
  std::string*        errorStringP
)
{
  Geometry                    geometry;
  std::vector<std::string>    pointStringV;
  int                         points;
  std::vector<orion::Point*>  pointV;

  type = (apiVersion == "v1")? FIWARE_LOCATION : FIWARE_LOCATION_V2;

  //
  // parse geometry
  //
  std::string errorString;
  if (geometry.parse(apiVersion, geometryString.c_str(), &errorString) != 0)
  {
    *errorStringP = std::string("error parsing geometry: ") + errorString;
    return -1;
  }


  //
  // Parse georel?
  //
  if (georelString != "")
  {
    if (georel.parse(georelString.c_str(), errorStringP) != 0)
    {
      return -1;
    }
  }

  // Check invalid combinations
  if ((geometry.areaType == "line") && (georel.type == "coveredBy"))
  {
    /* It seems that MongoDB 3.2 doesn't support this kind of queries, we get this error:
     *
     *  { $err: "Can't canonicalize query: BadValue $within not supported with provided geometry:
     *    { $geoWithin: { $geometry: { type: "LineString", coordinates: [ [ 5.0...", code: 17287 }
     */

    *errorStringP = "line geometry cannot be used with coveredBy georel";
    return -1;
  }

  if ((geometry.areaType == "point") && (georel.type == "coveredBy"))
  {
    /* It seems that MongoDB 3.2 doesn't support this kind of queries, we get this error:
     *
     *  { $err: "Can't canonicalize query: BadValue $within not supported with provided geometry:
     *    { $geoWithin: { $geometry: { type: "Point", coordinates: [ [ 5.0...", code: 17287 }
     */

    *errorStringP = "point geometry cannot be used with coveredBy georel";
    return -1;
  }

  if ((geometry.areaType != "point") && (georel.type == "near"))
  {
    /* It seems that MongoDB 3.2 doesn't support this kind of queries, we get this error:
     *
     *  { $err: "Can't canonicalize query: BadValue invalid point in geo near query $geometry argument:
     *   { type: "Polygon", coordinates: [ [ [ 2.0, 1.0 ], [ 4.0, 3.0 ],...", code: 17287 }
     */

    *errorStringP = "georel /near/ used with geometry different than point";
    return -1;
  }

  //
  // Split coordsString into a vector of points, or pairs of coordinates
  //
  if (coordsString == "")
  {
    *errorStringP = "no coordinates for geometry";
    return -1;
  }
  points = stringSplit(coordsString, ';', pointStringV);

  if (points == 0)
  {
    *errorStringP = "erroneous coordinates for geometry";
    return -1;
  }

  //
  // Convert point-strings into instances of the orion::Point class
  //
  for (int ix = 0; ix < points; ++ix)
  {
    std::vector<std::string>  coordV;
    int                       coords;
    double                    latitude;
    double                    longitude;

    coords = stringSplit(pointStringV[ix], ',', coordV);

    if (coords != 2)
    {
      *errorStringP = "invalid point in URI param /coords/";
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    if (!str2double(coordV[0].c_str(), &latitude))
    {      
      *errorStringP = "invalid coordinates";
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    if (!str2double(coordV[1].c_str(), &longitude))
    {
      *errorStringP = "invalid coordinates";
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    orion::Point* pointP = new Point(latitude, longitude);
    pointV.push_back(pointP);
  }


  if (geometry.areaType == "circle")
  {
    if (apiVersion == "v2")
    {
      *errorStringP = "circle geometry is not supported by Orion API v2";
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }
    else
    {
      if (pointV.size() != 1)
      {
        *errorStringP = "Too many coordinates for circle";
        pointVectorRelease(pointV);
        pointV.clear();
        return -1;
      }

      areaType = orion::CircleType;

      circle.radiusSet(geometry.radius);
      circle.invertedSet(geometry.external);
      circle.centerSet(pointV[0]);

      pointVectorRelease(pointV);
      pointV.clear();
    }
  }
  else if (geometry.areaType == "polygon")
  {
    areaType = orion::PolygonType;
    
    if ((apiVersion == "v1") && (pointV.size() < 3))
    {
      *errorStringP = "Too few coordinates for polygon";
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }
    else if ((apiVersion == "v2") && (pointV.size() < 4))
    {
      *errorStringP = "Too few coordinates for polygon";
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    //
    // If v2, first and last point must be identical
    //
    if ((apiVersion == "v2") && (pointV[0]->equals(pointV[pointV.size() - 1]) == false))
    {
      *errorStringP = "First and last point in polygon not the same";
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    polygon.invertedSet(geometry.external);

    for (unsigned int ix = 0; ix < pointV.size(); ++ix)
    {
      polygon.vertexAdd(pointV[ix]);
    }
    pointV.clear();
  }
  else if (geometry.areaType == "line")
  {
    areaType = orion::LineType;

    if (pointV.size() < 2)
    {
      *errorStringP = "invalid number of coordinates for /line/";
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    for (unsigned int ix = 0; ix < pointV.size(); ++ix)
    {
      line.pointAdd(pointV[ix]);
    }
    pointV.clear();
  }
  else if (geometry.areaType == "box")
  {
    areaType = orion::BoxType;

    if (pointV.size() != 2)
    {
      *errorStringP = "invalid number of coordinates for /box/";
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    // Check that points are different and not aligned (either horizontally or vertically)
    if ((pointV[0]->latitude() == pointV[1]->latitude())  || (pointV[0]->longitude() == pointV[1]->longitude()))
    {
      *errorStringP = "box coordinates are not defining an actual box";
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }

    double minLat = (pointV[0]->latitude()  < pointV[1]->latitude())?  pointV[0]->latitude()  : pointV[1]->latitude();
    double maxLat = (pointV[0]->latitude()  > pointV[1]->latitude())?  pointV[0]->latitude()  : pointV[1]->latitude();
    double minLon = (pointV[0]->longitude() < pointV[1]->longitude())? pointV[0]->longitude() : pointV[1]->longitude();
    double maxLon = (pointV[0]->longitude() > pointV[1]->longitude())? pointV[0]->longitude() : pointV[1]->longitude();

    // Lower left: smaller lat and long, upper right: greater lat and long
    Point ll;
    ll.latitudeSet(minLat);
    ll.longitudeSet(minLon);
    Point ur;
    ur.latitudeSet(maxLat);
    ur.longitudeSet(maxLon);
    box.fill(&ll, &ur);

    pointVectorRelease(pointV);
    pointV.clear();
  }
  else if (geometry.areaType == "point")
  {
    areaType = orion::PointType;

    if (pointV.size() != 1)
    {
      *errorStringP = "invalid number of coordinates for /point/";
      pointVectorRelease(pointV);
      pointV.clear();
      return -1;
    }
    point.fill(pointV[0]);

    pointVectorRelease(pointV);
    pointV.clear();
  }
  else
  {
    areaType = orion::NoArea;    
    *errorStringP = "invalid area-type";

    pointVectorRelease(pointV);
    pointV.clear();
    return -1;
  }

  return 0;
}



/* ****************************************************************************
*
* Scope::render -
*/
std::string Scope::render(Format format, const std::string& indent, bool notLastInVector)
{
  std::string out      = "";
  std::string tag      = "operationScope";
  const char* tTag     = (format == XML)? "scopeType"  : "type";
  const char* vTag     = (format == XML)? "scopeValue" : "value";

  out += startTag(indent, tag, tag, format, false, false);
  out += valueTag(indent + "  ", tTag, type, format, true);
  out += valueTag(indent + "  ", vTag, value, format);
  out += endTag(indent, tag, format, notLastInVector);

  return out;
}



/* ****************************************************************************
*
* Scope::check -
*/
std::string Scope::check
(
  RequestType         requestType,
  Format              format,
  const std::string&  indent,
  const std::string&  predetectedError,
  int                 counter
)
{
  //
  // Check for forbidden characters
  //
  if (forbiddenChars(type.c_str()))
  {
    alarmMgr.badInput(clientIp, "found a forbidden character in the type of a scope");
    return "illegal chars in scope type";
  }

  if (type != SCOPE_TYPE_SIMPLE_QUERY)
  {
    if (forbiddenChars(value.c_str()))
    {
      alarmMgr.badInput(clientIp, "found a forbidden character in the value of a scope");
      return "illegal chars in scope";
    }
  }

  if (type == FIWARE_LOCATION || type == FIWARE_LOCATION_DEPRECATED)
  {
    if (areaType == orion::CircleType)
    {
      if (circle.radiusString() == "0")
      {
        alarmMgr.badInput(clientIp, "radius zero for a circle area");
        return "Radius zero for a circle area";
      }
      else if (circle.radiusString() == "")
      {
        alarmMgr.badInput(clientIp, "missing radius for circle area");
        return "Missing radius for circle area";
      }
      else if (circle.invertedString() != "")
      {
        if (!isTrue(circle.invertedString()) && !isFalse(circle.invertedString()))
        {
          std::string details = std::string("bad value for circle/inverted: '") + circle.invertedString() + "'"; 
          alarmMgr.badInput(clientIp, details);
          return "bad value for circle/inverted: /" + circle.invertedString() + "/";
        }
      }
      else if (circle.center.latitudeString() == "")
      {
        alarmMgr.badInput(clientIp, "missing latitude for circle center");
        return "Missing latitude for circle center";
      }
      else if (circle.center.longitudeString() == "")
      {
        alarmMgr.badInput(clientIp, "missing longitude for circle center");
        return "Missing longitude for circle center";
      }

      double latitude;
      double longitude;
      bool   ok;

      ok = str2double(circle.center.latitudeString().c_str(), &latitude);
      if ((ok == false) || (latitude > 90) || (latitude < -90))
      {
        std::string details = std::string("invalid value for latitude (") + circle.center.latitudeString() + ")";
        alarmMgr.badInput(clientIp, details);
        return "invalid value for latitude";
      }

      ok = str2double(circle.center.longitudeString().c_str(), &longitude);
      if ((ok == false) || (longitude > 180) || (longitude < -180))
      {
        std::string details = std::string("invalid value for longitude: '") + circle.center.longitudeString() + "'";
        alarmMgr.badInput(clientIp, details);
        return "invalid value for longitude";
      }
    }
    else if (areaType == orion::PolygonType)
    {
      if (polygon.vertexList.size() < 3)
      {
        char noOfV[STRING_SIZE_FOR_INT];

        snprintf(noOfV, sizeof(noOfV), "%lu", polygon.vertexList.size());
        std::string details = std::string("too few vertices for a polygon (") + noOfV + " is less than three)";
        alarmMgr.badInput(clientIp, details);

        return "too few vertices for a polygon";
      }
      else if (polygon.invertedString() != "")
      {
        if (!isTrue(polygon.invertedString()) && !isFalse(polygon.invertedString()))
        {
          std::string details = std::string("bad value for polygon/inverted: '") + polygon.invertedString() + "'";
          alarmMgr.badInput(clientIp, details);
          return "bad value for polygon/inverted: /" + polygon.invertedString() + "/";
        }
      }

      for (unsigned int ix = 0; ix < polygon.vertexList.size(); ++ix)
      {
        if (polygon.vertexList[ix]->latitudeString() == "")
        {
          alarmMgr.badInput(clientIp, "missing latitude value for polygon vertex");
          return std::string("missing latitude value for polygon vertex");
        }

        if (polygon.vertexList[ix]->longitudeString() == "")
        {
          alarmMgr.badInput(clientIp, "missing longitude value for polygon vertex");
          return std::string("missing longitude value for polygon vertex");
        }

        double latitude;
        double longitude;
        bool   ok;


        ok = str2double(polygon.vertexList[ix]->latitudeString().c_str(), &latitude);
        if ((ok == false) || (latitude > 90) || (latitude < -90))
        {
          std::string details = std::string("invalid value for latitude: '") + polygon.vertexList[ix]->latitudeString() + "'";
          alarmMgr.badInput(clientIp, details);
          return "invalid value for latitude";
        }

        ok = str2double(polygon.vertexList[ix]->longitudeString().c_str(), &longitude);
        if ((ok == false) || (longitude > 180) || (longitude < -180))
        {
          std::string details = std::string("invalid value for longitude: '") + polygon.vertexList[ix]->longitudeString() + "'";
          alarmMgr.badInput(clientIp, details);
          return "invalid value for longitude";
        }
      }
    }
  }

  if ((type != FIWARE_LOCATION) && (type != FIWARE_LOCATION_DEPRECATED))
  {
    if (type == "")
    {
      alarmMgr.badInput(clientIp, "empty type in restriction scope");
      return "Empty type in restriction scope";
    }

    if (value == "")
    {
      alarmMgr.badInput(clientIp, "empty value in restriction scope");
      return "Empty value in restriction scope";
    }
  }

  if (type == FIWARE_LOCATION_V2)
  {
    if ((areaType == orion::PointType) && (georel.type == "coveredBy"))
    {
      alarmMgr.badInput(clientIp, "Query not supported: point geometry cannot be used with coveredBy georel");
      return "Query not supported: point geometry cannot be used with coveredBy georel";
    }
    else if ((areaType == orion::LineType) && (georel.type == "coveredBy"))
    {
      alarmMgr.badInput(clientIp, "Query not supported: line  geometry cannot be used with coveredBy georel");
      return "Query not supported: line geometry cannot be used with coveredBy georel";
    }
    else if ((areaType == orion::LineType) && (line.pointList.size() < 2))
    {
      alarmMgr.badInput(clientIp, "Query not supported: not enough points for a line");
      return "Query not supported: not enough points for a line";
    }
    else if ((areaType == orion::PolygonType) && (polygon.vertexList.size() < 4))
    {
      alarmMgr.badInput(clientIp, "Query not supported: not enough vertices for a polygon");
      return "Query not supported: not enough vertices for a polygon";
    }
  }

  return "OK";
}



/* ****************************************************************************
*
* Scope::present -
*/
void Scope::present(const std::string& indent, int ix)
{
  if (ix == -1)
  {
    LM_T(LmtPresent, ("%sScope:",       indent.c_str()));
  }
  else
  {
    LM_T(LmtPresent, ("%sScope %d:",    indent.c_str(), ix));
  }

  LM_T(LmtPresent, ("%s  Type:     '%s'", indent.c_str(), type.c_str()));
  
  if (oper != "")
    LM_T(LmtPresent, ("%s  Operator: '%s'", indent.c_str(), oper.c_str()));

  if (areaType == orion::NoArea)
  {
    LM_T(LmtPresent, ("%s  Value:    %s", indent.c_str(), value.c_str()));
  }
  else if (areaType == orion::CircleType)
  {
    LM_T(LmtPresent, ("%s  FI-WARE Circle Area:", indent.c_str()));
    LM_T(LmtPresent, ("%s    Radius:     %s",     indent.c_str(), circle.radiusString().c_str()));
    LM_T(LmtPresent, ("%s    Longitude:  %s",     indent.c_str(), circle.center.longitudeString().c_str()));
    LM_T(LmtPresent, ("%s    Latitude:   %s", 	  indent.c_str(), circle.center.latitudeString().c_str()));
    LM_T(LmtPresent, ("%s    Inverted:   %s", 	  indent.c_str(), circle.invertedString().c_str()));
  }
  else if (areaType == orion::PolygonType)
  {
    LM_T(LmtPresent, ("%s  FI-WARE Polygon Area (%lu vertices):", indent.c_str(), polygon.vertexList.size()));

    LM_T(LmtPresent, ("%s    Inverted:   %s", indent.c_str(), polygon.invertedString().c_str()));
    for (unsigned int ix = 0; ix < polygon.vertexList.size(); ++ix)
    {
      LM_T(LmtPresent, ("%s    Vertex %d",        indent.c_str(), ix));
      LM_T(LmtPresent, ("%s      Longitude:  %s", indent.c_str(), polygon.vertexList[ix]->longitudeString().c_str()));
      LM_T(LmtPresent, ("%s      Latitude:   %s", indent.c_str(), 
			polygon.vertexList[ix]->latitudeString().c_str()));
    }
  }
  else if (areaType == orion::PointType)
  {
    LM_T(LmtPresent, ("%s  FI-WARE Point:", indent.c_str()));
    LM_T(LmtPresent, ("%s      Longitude:  %s", indent.c_str(), point.longitudeString().c_str()));
    LM_T(LmtPresent, ("%s      Latitude:   %s", indent.c_str(), point.latitudeString().c_str()));
  }
  else if (areaType == orion::BoxType)
  {
    LM_T(LmtPresent, ("%s  FI-WARE Box:", indent.c_str()));
    LM_T(LmtPresent, ("%s      lowerLeft.longitude:   %s", indent.c_str(), box.lowerLeft.longitudeString().c_str()));
    LM_T(LmtPresent, ("%s      lowerLeft.latitude:    %s", indent.c_str(), box.lowerLeft.latitudeString().c_str()));
    LM_T(LmtPresent, ("%s      upperRight.longitude:  %s", indent.c_str(), box.upperRight.longitudeString().c_str()));
    LM_T(LmtPresent, ("%s      upperRight.latitude:   %s", indent.c_str(), box.upperRight.latitudeString().c_str()));
  }
  else if (areaType == orion::LineType)
  {
    LM_T(LmtPresent, ("%s  FI-WARE Line:", indent.c_str()));
    for (unsigned int ix = 0; ix < line.pointList.size(); ++ix)
    {
      LM_T(LmtPresent, ("%s    Point %d",        indent.c_str(), ix));
      LM_T(LmtPresent, ("%s      Longitude:  %s", indent.c_str(), line.pointList[ix]->longitudeString().c_str()));
      LM_T(LmtPresent, ("%s      Latitude:   %s", indent.c_str(),
      line.pointList[ix]->latitudeString().c_str()));
    }
  }
  else
  {
    LM_T(LmtPresent, ("%s  Unknown areaType '%d'", indent.c_str(), areaType));
  }

  if (georel.type == "near")
  {
    LM_T(LmtPresent, ("%s  Georel: 'near'", indent.c_str()));
    LM_T(LmtPresent, ("%s    maxDistance:  %f", georel.maxDistance));
    LM_T(LmtPresent, ("%s    minDistance:  %f", georel.minDistance));
  }
  else if ((georel.type == "coveredBy") || (georel.type == "intersects") || (georel.type == "equals") || (georel.type == "disjoint"))
  {
    LM_T(LmtPresent, ("%s  Georel: '%s'", indent.c_str(), georel.type.c_str()));
  }
}



/* ****************************************************************************
*
* release -
*/
void Scope::release(void)
{  
  // note that georel, circle, box, point don't use dynamic memory, so they don't need release methods
  polygon.release();
  line.release();
}



/* ****************************************************************************
*
* Scope::areaTypeSet -
*/
void Scope::areaTypeSet(const std::string& areaTypeString)
{
  if      (areaTypeString == "line")    areaType = orion::LineType;
  else if (areaTypeString == "polygon") areaType = orion::PolygonType;
  else if (areaTypeString == "circle")  areaType = orion::CircleType;
  else if (areaTypeString == "point")   areaType = orion::PointType;
  else if (areaTypeString == "box")     areaType = orion::BoxType;
  else                                  areaType = orion::NoArea;       
}
