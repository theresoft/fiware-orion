#ifndef REGISTER_CONTEXT_REQUEST_H
#define REGISTER_CONTEXT_REQUEST_H

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
#include "convenience/RegisterProviderRequest.h"
#include "ngsi/ContextRegistrationVector.h"
#include "ngsi/Duration.h"
#include "ngsi/RegistrationId.h"



/* ****************************************************************************
*
* RegisterContextRequest - 
*/
typedef struct RegisterContextRequest
{
  ContextRegistrationVector  contextRegistrationVector;  // Mandatory
  Duration                   duration;                   // Optional
  RegistrationId             registrationId;             // Optional

  std::string                servicePath;                // Not part of payload, just an internal field

  std::string   render(RequestType requestType, Format format, const std::string& indent);
  std::string   check(ConnectionInfo* ciP, RequestType requestType, Format format, const std::string& indent, const std::string& predetectedError, int counter);
  void          present(void);
  void          release(void);
  void          fill(RegisterProviderRequest& rpr, const std::string& entityId, const std::string& entityType, const std::string& attributeName);
} RegisterContextRequest;

#endif
