#include <iostream>
#include <stdio.h>
#include <string>
#include <sstream>
#include "rapidjson/document.h"

#include "ngsi/EntityId.h"
#include "ngsi/ContextAttribute.h"
#include "ngsi/ContextElement.h"
#include "ngsi/Metadata.h"

using namespace rapidjson;

/* Let's use as input a string containing a JSON. In a real case, the rest layer would provide this input.
 * The example is based on the request payload for a create entity operation*/
std::string input = " {"
     "   \"type\": \"Car\","
     "   \"id\": \"P-9873-K\","
     "   \"kms\": 159675,"
     "   \"fuel\": 23.5,"
     "   \"driver\": \"Fermin\","
     "   \"speed\": {"
     "     \"value\": 100,"
     "     \"type\": \"number\","
     "     \"accuracy\": 2,"
     "     \"timestamp\": {"
     "       \"value\": \"2015-06-04T07:20:27.378Z\","
     "       \"type\": \"date\""
     "     }"
     "   }"
     " }";

static const char* kTypeNames[] =
    { "Null", "False", "True", "Object", "Array", "String", "Number" };

void processObjectMetadata(Metadata* mdP, const Value& v)
{
  for (Value::ConstMemberIterator itr = v.MemberBegin(); itr != v.MemberEnd(); ++itr)
  {
    std::string name = itr->name.GetString();
    std::string type = std::string(kTypeNames[itr->value.GetType()]);

    /* Get attribute type */
    if (name == "type")
    {
      mdP->type = itr->value.GetString();
      continue;
    }

    /* Get attribute value */
    if (name == "value")
    {
      /* Number or string? */
      if (type == "Number")
      {
        std::ostringstream strs;
        strs << itr->value.GetDouble();
        mdP->value = strs.str();
      }
      else
      {
        mdP->value = itr->value.GetString();
      }
      continue;
    }
  }
}

void processObjectAttribute(ContextAttribute* caP, const Value& v)
{
  /* We would have to check the existence of a "value" subfield. In that case, then it is a
   * regular attribute with type and/or metadata. Otherwise, the JSON object itself is the value
   * of the attribute. To keep things simpler, this PoC assumes that it is always in the first case,
   * we are not doing that checking */
  for (Value::ConstMemberIterator itr = v.MemberBegin(); itr != v.MemberEnd(); ++itr)
  {
    std::string name = itr->name.GetString();
    std::string type = std::string(kTypeNames[itr->value.GetType()]);

    /* Get attribute type */
    if (name == "type")
    {
      caP->type = itr->value.GetString();
      continue;
    }

    /* Get attribute value */
    if (name == "value")
    {
      /* Number or string? */
      if (type == "Number")
      {
        std::ostringstream strs;
        strs << itr->value.GetDouble();
        caP->value = strs.str();
      }
      else
      {
        caP->value = itr->value.GetString();
      }
      continue;
    }

    /* All the fiels not "type" or "value" are metadata, by definition */
    Metadata* mdP = new Metadata();
    mdP->name = name;
    if (type == "Object")
    {
      /* In this case, the metadata is represented by a complete JSON object, that need to be processed */
      processObjectMetadata(mdP, itr->value);
    }
    else if (type == "Number")
    {
      mdP->type  = "number";
      std::ostringstream strs;
      strs << itr->value.GetDouble();
      mdP->value = strs.str();
    }
    else
    {
      /* Simple string */
      mdP->type  = "";
      mdP->value = itr->value.GetString();
    }

    caP->metadataVector.push_back(mdP);
  }
}

int main(int argC, char* argV[])
{

  Document document;
  document.Parse(input.c_str());

  assert(document.IsObject());

  ContextElement ce;

  /* Let's start analyzing the first level */
  std::string id;
  std::string enType;
  for (Value::ConstMemberIterator itr = document.MemberBegin(); itr != document.MemberEnd(); ++itr)
  {
    std::string name = itr->name.GetString();
    std::string type = std::string(kTypeNames[itr->value.GetType()]);

    /* Get entity id */
    if (name == "id")
    {
      id = itr->value.GetString();
      continue;
    }

    /* Get entity type */
    if (name == "type")
    {
      enType = itr->value.GetString();
      continue;
    }

    /* Any other field at this level is, by definition, an attribute */
    ContextAttribute* caP = new ContextAttribute();
    caP->name = name;
    /* We have to analyze the field type in order to know what to do */
    if (type == "Vector")
    {
      /* If type is a vector, then it is directly the attribute value. At this point, the vector should be
       * processed and the compoundValueP field in ContextAttribute used. For this PoC is enough to set
       * "__vector" */
      caP->type  = "";
      caP->value = "__vector";
    }
    else if (type == "Object")
    {
      /* In this case, the attribute is represented by a complete JSON object, that need to be processed */
      processObjectAttribute(caP, itr->value);
    }
    else if (type == "Number")
    {
      caP->type  = "number";
      std::ostringstream strs;
      strs << itr->value.GetDouble();
      caP->value = strs.str();
    }
    else
    {
      /* Simple string */
      caP->type  = "";
      caP->value = itr->value.GetString();
    }
    ce.contextAttributeVector.push_back(caP);
  }

  ce.entityId.fill(id, enType, "");

  /* Now the NGSI object is ready to be passed to the next level (serviceRoutine). In this PoC, we
   * just print it to see that it has been built succesfully */
  ce.present("", 0);

  return 0;
}
