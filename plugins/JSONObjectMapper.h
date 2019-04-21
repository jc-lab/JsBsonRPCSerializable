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
 * @file	JSONObjectMapper.h
 * @author	Jichan (development@jc-lab.net / http://ablog.jc-lab.net/ )
 * @date	2019/04/10
 * @copyright Copyright (C) 2018 jichan.\n
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */

#pragma once

#include "../Serializable.h"

#if defined(HAS_RAPIDJSON) && HAS_RAPIDJSON
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#endif

#include <string>

namespace JsBsonRPC {

	class JSONObjectMapper
	{
	public:
		class TypeNotSupportException : public std::exception
		{ };
		class ConvertException : public std::exception
		{ };

	private:
		struct ConvertContext : public internal::BsonParseHandler {
			internal::BsonTypes bsonType;
			rapidjson::Document &doc;
			ConvertContext(rapidjson::Document &_document, internal::BsonTypes _bsonType) : doc(_document), bsonType(_bsonType) {
				if (_bsonType == internal::BSONTYPE_DOCUMENT)
					doc.SetObject();
				else if (_bsonType == internal::BSONTYPE_ARRAY)
					doc.SetArray();
			}
			bool bsonParseHandle(uint8_t type, const std::string &name, const std::vector<unsigned char>& payload, uint32_t *offset, uint32_t docEndPos) throw(TypeNotSupportException, ConvertException) override;
		};

	public:
#if defined(HAS_RAPIDJSON) && HAS_RAPIDJSON
		void serializeTo(const Serializable *serialiable, rapidjson::Document &jsonDoc) throw(TypeNotSupportException, ConvertException);
		void deserializeJsonObject(Serializable *serialiable, const rapidjson::Value &jsonObject) throw(TypeNotSupportException, ConvertException);

		std::string serialize(const Serializable *serialiable) throw(TypeNotSupportException, ConvertException)
		{
			rapidjson::Document jsonDoc;
			rapidjson::StringBuffer jsonBuf;
			rapidjson::Writer<rapidjson::StringBuffer> jsonWriter(jsonBuf);
			serializeTo(serialiable, jsonDoc);
			jsonDoc.Accept(jsonWriter);
			return std::string(jsonBuf.GetString(), jsonBuf.GetLength());
		}

		void deserialize(Serializable *serialiable, const std::string &json) throw(TypeNotSupportException, ConvertException)
		{
			rapidjson::Document jsonDoc;
			jsonDoc.Parse(json.c_str(), json.length());
			deserializeJsonObject(serialiable, jsonDoc);
		}
#endif
	};

}
