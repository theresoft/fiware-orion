#ifndef SRC_LIB_NGSI_METADATA_H_
#define SRC_LIB_NGSI_METADATA_H_

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
#include <vector>

#include "common/Format.h"
#include "orionTypes/OrionValueType.h"
#include "ngsi/Request.h"

#include "mongo/client/dbclient.h"



/* ****************************************************************************
*
* Defines -
*
* Metadata interpreted by Orion Context Broker, i.e. not custom metadata
*/
#define NGSI_MD_ID       "ID"
#define NGSI_MD_LOCATION "location"
#define NGSI_MD_CREDATE  "creDate"    // FIXME P5: to be used for creDate (currenly only in DB)
#define NGSI_MD_MODDATE  "modDate"    // FIXME P5: to be used for modDate (currenly only in DB)



/* ****************************************************************************
*
* Metadata -
*
*/
typedef struct Metadata
{
  std::string  name;         // Mandatory
  std::string  type;         // Optional

  // Mandatory
  orion::ValueType   valueType;    // Type of value: taken from JSON parse
  std::string        stringValue;  // "value" as a String
  double             numberValue;  // "value" as a Number
  bool               boolValue;    // "value" as a Boolean
  bool               typeGiven;    // Was 'type' part of the incoming payload?

  Metadata();
  Metadata(Metadata* mP, bool useDefaultType = false);
  Metadata(const std::string& _name, const std::string& _type, const char* _value);
  Metadata(const std::string& _name, const std::string& _type, const std::string& _value);
  Metadata(const std::string& _name, const std::string& _type, double _value);
  Metadata(const std::string& _name, const std::string& _type, bool _value);
  Metadata(const mongo::BSONObj& mdB);

  std::string  render(Format format, const std::string& indent, bool comma = false);
  std::string  toJson(bool isLastElement);
  void         present(const std::string& metadataType, int ix, const std::string& indent);
  void         release(void);
  void         fill(const struct Metadata& md);
  std::string  toStringValue(void) const;

  std::string  check(ConnectionInfo*     ciP,
                     RequestType         requestType,
                     Format              format,
                     const std::string&  indent,
                     const std::string&  predetectedError,
                     int                 counter);
} Metadata;

#endif  // SRC_LIB_NGSI_METADATA_H_
