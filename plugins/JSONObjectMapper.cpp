/*
* Licensed to the Apache Software Foundation (ASF) under one or more
* contributor license agreements.  See the NOTICE file distributed with
* this work for additional information regarding copyright ownership.
* The ASF licenses this file to You under the Apache License, Version 2.0
* (the "License"); you may not use this file except in compliance with
* the License.  You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
/**
 * @file	JSONObjectMapper.cpp
 * @author	Jichan (development@jc-lab.net / http://ablog.jc-lab.net/ )
 * @date	2019/04/10
 * @copyright Copyright (C) 2018 jichan.\n
 *             This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */
 
#include "JSONObjectMapper.h"

#if defined(HAS_RAPIDJSON) && HAS_RAPIDJSON
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#if defined(HAS_JSCPPUTILS) && HAS_JSCPPUTILS
#include <JsCPPUtils/StringBuffer.h>
#include <JsCPPUtils/StringEncoding.h>
#include <JsCPPUtils/Base64.h>
#endif

namespace JsBsonRPC {

#if defined(HAS_RAPIDJSON) && HAS_RAPIDJSON
	template<typename T>
	static uint32_t writeValueToBson(std::vector<unsigned char> &payload, const T& value);

	template<>
	static uint32_t writeValueToBson<uint64_t>(std::vector<unsigned char> &payload, const uint64_t& value) {
		int i;
		const unsigned char *carr = (const unsigned char*)&value;
		for (i = 0; i < sizeof(uint64_t); i++)
			payload.push_back(carr[i]);
		return i;
	}
	template<>
	static uint32_t writeValueToBson<int64_t>(std::vector<unsigned char> &payload, const int64_t& value) {
		int i;
		const unsigned char *carr = (const unsigned char*)&value;
		for (i = 0; i < sizeof(int64_t); i++)
			payload.push_back(carr[i]);
		return i;
	}
	template<>
	static uint32_t writeValueToBson<uint32_t>(std::vector<unsigned char> &payload, const uint32_t& value) {
		int i;
		const unsigned char *carr = (const unsigned char*)&value;
		for (i = 0; i < sizeof(uint32_t); i++)
			payload.push_back(carr[i]);
		return i;
	}
	template<>
	static uint32_t writeValueToBson<int32_t>(std::vector<unsigned char> &payload, const int32_t& value) {
		int i;
		const unsigned char *carr = (const unsigned char*)&value;
		for (i = 0; i < sizeof(int32_t); i++)
			payload.push_back(carr[i]);
		return i;
	}
	template<>
	static uint32_t writeValueToBson<double>(std::vector<unsigned char> &payload, const double& value) {
		int i;
		const unsigned char *carr = (const unsigned char*)&value;
		for (i = 0; i < sizeof(double); i++)
			payload.push_back(carr[i]);
		return i;
	}

	static uint32_t writeKeyToBson(std::vector<unsigned char> &payload, const std::string &key)
	{
		payload.insert(payload.end(), key.begin(), key.end());
		payload.push_back(0);
		return key.length() + 1;
	}

	static uint32_t jsonArrayToBson(std::vector<unsigned char> &payload, const std::string& key, const rapidjson::Value &jsonObject);
	static uint32_t jsonObjectToBson(std::vector<unsigned char> &payload, const std::string& key, const rapidjson::Value &jsonObject);

	static uint32_t addJsonValueToBson(std::vector<unsigned char> &payload, const std::string &key, const rapidjson::Value& jsonValue)
	{
		uint32_t payloadSize = 1;
		uint8_t bsonType = 0;
		if (jsonValue.IsInt64()) {
			payload.push_back(internal::BSONTYPE_INT64);
			payloadSize += writeKeyToBson(payload, key);
			payloadSize += writeValueToBson<int64_t>(payload, jsonValue.GetInt64());
		}
		else if (jsonValue.IsUint64()) {
			payload.push_back(internal::BSONTYPE_INT64);
			payloadSize += writeKeyToBson(payload, key);
			payloadSize += writeValueToBson<uint64_t>(payload, jsonValue.GetUint64());
		}
		else if (jsonValue.IsInt()) {
			payload.push_back(internal::BSONTYPE_INT32);
			payloadSize += writeKeyToBson(payload, key);
			payloadSize += writeValueToBson<int32_t>(payload, jsonValue.GetInt());
		}
		else if (jsonValue.IsUint()) {
			payload.push_back(internal::BSONTYPE_INT32);
			payloadSize += writeKeyToBson(payload, key);
			payloadSize += writeValueToBson<uint32_t>(payload, jsonValue.GetUint());
		}
		else if (jsonValue.IsString()) {
			uint32_t len = jsonValue.GetStringLength() + 1;
			payload.push_back(internal::BSONTYPE_STRING_UTF8);
			payloadSize += writeKeyToBson(payload, key);
			payload.push_back((unsigned char)(len >> 0));
			payload.push_back((unsigned char)(len >> 8));
			payload.push_back((unsigned char)(len >> 16));
			payload.push_back((unsigned char)(len >> 24));
			payload.insert(payload.end(), jsonValue.GetString(), jsonValue.GetString() + jsonValue.GetStringLength());
			payload.push_back(0);
			payloadSize += 4 + len;
		}
		else if (jsonValue.IsDouble()) {
			payload.push_back(internal::BSONTYPE_DOUBLE);
			payloadSize += writeKeyToBson(payload, key);
			payloadSize += writeValueToBson<double>(payload, jsonValue.GetDouble());
		}
		else if (jsonValue.IsFloat()) {
			payload.push_back(internal::BSONTYPE_DOUBLE);
			payloadSize += writeKeyToBson(payload, key);
			payloadSize += writeValueToBson<double>(payload, (double)jsonValue.GetFloat());
		}
		else if (jsonValue.IsArray()) {
			payloadSize = jsonArrayToBson(payload, key, jsonValue);
		}
		else if (jsonValue.IsObject()) {
			payloadSize = jsonObjectToBson(payload, key, jsonValue);
		}
		else if (jsonValue.IsNull()) {
			payload.push_back(internal::BSONTYPE_NULL);
			payloadSize += writeKeyToBson(payload, key);
		}
		else {
			throw JSONObjectMapper::TypeNotSupportException();
		}
		return payloadSize;
	}

	static uint32_t jsonArrayToBson(std::vector<unsigned char> &payload, const std::string& key, const rapidjson::Value &jsonObject)
	{
		uint32_t payloadSize = 1;
		uint32_t subDocSize = 5;
		uint32_t headOffset = 0;
		int i = 0;
		payload.push_back(internal::BSONTYPE_ARRAY);
		payloadSize += writeKeyToBson(payload, key);
		headOffset = payload.size();
		payload.push_back(0); payload.push_back(0); payload.push_back(0); payload.push_back(0);
		for (rapidjson::Value::ConstValueIterator iter = jsonObject.Begin(); iter != jsonObject.End(); iter++)
		{
			char key[32] = {0};
			internal::_my_itoa(i, key, sizeof(key), 10);
			subDocSize += addJsonValueToBson(payload, key, *iter);
			i++;
		}
		payload.push_back(0);
		payload[headOffset + 0] = (unsigned char)(subDocSize >> 0);
		payload[headOffset + 1] = (unsigned char)(subDocSize >> 8);
		payload[headOffset + 2] = (unsigned char)(subDocSize >> 16);
		payload[headOffset + 3] = (unsigned char)(subDocSize >> 24);
		return payloadSize + subDocSize;
	}
	static uint32_t jsonObjectToBson(std::vector<unsigned char> &payload, const std::string& key, const rapidjson::Value &jsonObject)
	{
		uint32_t payloadSize = 0;
		uint32_t subDocSize = 5;
		uint32_t headOffset = 0;
		if (!key.empty()) {
			payload.push_back(internal::BSONTYPE_DOCUMENT);
			payloadSize = 1 + writeKeyToBson(payload, key);
		}
		headOffset = payload.size();
		payload.push_back(0); payload.push_back(0); payload.push_back(0); payload.push_back(0);
		for (rapidjson::Value::ConstMemberIterator iter = jsonObject.MemberBegin(); iter != jsonObject.MemberEnd(); iter++)
		{
			subDocSize += addJsonValueToBson(payload, std::string(iter->name.GetString(), iter->name.GetStringLength()), iter->value);
		}
		payload.push_back(0);
		payload[headOffset + 0] = (unsigned char)(subDocSize >> 0);
		payload[headOffset + 1] = (unsigned char)(subDocSize >> 8);
		payload[headOffset + 2] = (unsigned char)(subDocSize >> 16);
		payload[headOffset + 3] = (unsigned char)(subDocSize >> 24);
		return payloadSize + subDocSize;
	}

	void JSONObjectMapper::serializeTo(const Serializable *serialiable, rapidjson::Document &jsonDoc) throw(TypeNotSupportException, ConvertException)
	{
		ConvertContext convertContext(jsonDoc, internal::BSONTYPE_DOCUMENT);
		std::vector<unsigned char> bsonPayload;
		uint32_t offset = 0;
		serialiable->serialize(bsonPayload);
		internal::BsonParser parser(bsonPayload, bsonPayload.size(), &offset);
		parser.parse(&convertContext);
	}

	void JSONObjectMapper::deserializeJsonObject(Serializable *serialiable, const rapidjson::Value &jsonObject) throw(TypeNotSupportException, ConvertException)
	{
		std::vector<unsigned char> payload;
		uint32_t docSize;
		if (jsonObject.IsObject())
			docSize = jsonObjectToBson(payload, "", jsonObject);
		else
			throw ConvertException();
		serialiable->deserialize(payload);
	}

	void JSONObjectMapper::ConvertContext::bsonParseHandle(uint8_t type, const std::string &name, const std::vector<unsigned char>& payload, uint32_t *offset, uint32_t docEndPos) throw(TypeNotSupportException, ConvertException)
	{
		rapidjson::Value jsonKey;
		rapidjson::Value jsonValue;
		jsonKey.SetString(name.c_str(), name.length(), doc.GetAllocator());

		switch(type)
		{
		case internal::BSONTYPE_DOUBLE:
			jsonValue.SetDouble(internal::readValue<double>(payload, offset, docEndPos));
			break;
		case internal::BSONTYPE_STRING_UTF8:
		{
			uint32_t len = internal::readValue<uint32_t>(payload, offset, docEndPos);
			if ((docEndPos - *offset) < len)
				throw ConvertException();
			if (payload[*offset + len - 1] == 0)
				jsonValue.SetString((const char*)&payload[*offset], len - 1, doc.GetAllocator());
			else
				jsonValue.SetString((const char*)&payload[*offset], len, doc.GetAllocator());
			*offset += len;
		}
			break;
		case internal::BSONTYPE_DOCUMENT:
		{
			rapidjson::Document subDoc;
			ConvertContext convertContext(subDoc, internal::BSONTYPE_DOCUMENT);
			internal::BsonParser parser(payload, payload.size(), offset);
			parser.parse(&convertContext);
			jsonValue.CopyFrom(convertContext.doc, doc.GetAllocator());
		}
			break;
		case internal::BSONTYPE_ARRAY:
		{
			rapidjson::Document subDoc;
			ConvertContext convertContext(subDoc, internal::BSONTYPE_ARRAY);
			internal::BsonParser parser(payload, payload.size(), offset);
			parser.parse(&convertContext);
			jsonValue.CopyFrom(convertContext.doc, doc.GetAllocator());
		}
			break;
#if defined(HAS_JSCPPUTILS) && HAS_JSCPPUTILS
		case internal::BSONTYPE_BINARY:
		{
			uint32_t len = internal::readValue<uint32_t>(payload, offset, docEndPos);
			uint8_t value = internal::readValue<uint8_t>(payload, offset, docEndPos);
			std::string encoded = JsCPPUtils::Base64::encodeToText((const char*)&payload[*offset], len);
			*offset += len;
			jsonValue.SetString(encoded.c_str(), encoded.length(), doc.GetAllocator());
		}
			break;
#endif
		case internal::BSONTYPE_BOOL:
			jsonValue.SetBool(internal::readValue<unsigned char>(payload, offset, docEndPos) ? true : false);
			break;
		case internal::BSONTYPE_UTCDATETIME:
			jsonValue.SetUint64(internal::readValue<uint64_t>(payload, offset, docEndPos));
			break;
		case internal::BSONTYPE_NULL:
			jsonValue.SetNull();
			break;
		case internal::BSONTYPE_INT32:
			jsonValue.SetInt(internal::readValue<int32_t>(payload, offset, docEndPos));
			break;
		case internal::BSONTYPE_TIMESTAMP:
			jsonValue.SetUint64(internal::readValue<uint64_t>(payload, offset, docEndPos));
			break;
		case internal::BSONTYPE_INT64:
			jsonValue.SetInt64(internal::readValue<int64_t>(payload, offset, docEndPos));
			break;
		default:
			throw TypeNotSupportException();
		}

		if (bsonType == internal::BSONTYPE_ARRAY)
		{
			doc.PushBack(jsonValue, doc.GetAllocator());
		}else{
			doc.AddMember(jsonKey, jsonValue, doc.GetAllocator());
		}
	}
#endif

}

#endif
