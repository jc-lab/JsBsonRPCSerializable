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
 * @file	Serializable.cpp
 * @author	Jichan (development@jc-lab.net / http://ablog.jc-lab.net/ )
 * @date	2019/04/10
 * @copyright Copyright (C) 2018 jichan.\n
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */
 
#include "Serializable.h"

namespace JsBsonRPC {

	namespace internal {
		int _my_itoa(
			int    value,
			char*  buf, \
			size_t bufSize,
			int    radix
		)
		{
#if defined(WIN32) && defined(_MSC_VER)
			return _itoa_s(value, buf, bufSize, radix);
#else
			if (itoa(value, buf, radix))
				return 0;
			return 1;
#endif
		}

		uint32_t serializeKey(std::vector<unsigned char> &payload, const std::string& key) {
			payload.insert(payload.end(), key.begin(), key.end());
			payload.push_back(0);
			return key.length() + 1;
		}
	}

	Serializable::Serializable(const char *name, int64_t serialVersionUID)
	{
		m_name = name;
		m_serialVersionUID = serialVersionUID;
	}

	Serializable::~Serializable()
	{

	}

	internal::STypeCommon &Serializable::serializableMapMember(const char *name, internal::STypeCommon &object)
	{
		object.setMemberName(name);
		m_members.push_back(&object);
		return object;
	}

	void Serializable::serializableClearObjects()
	{
		for (std::list<internal::STypeCommon*>::const_iterator iter = m_members.begin(); iter != m_members.end(); iter++)
		{
			(*iter)->clear();
		}
	}

	size_t Serializable::serialize(std::vector<unsigned char>& payload) const throw(UnavailableTypeException)
	{
		size_t offset = 0;
		offset = payload.size();
		// DOCUMENT HEADER : SIZE
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		payload.push_back(0);
		uint32_t totalSize = 5;
		for (std::list<internal::STypeCommon*>::const_iterator iterMem = m_members.begin(); iterMem != m_members.end(); iterMem++)
		{
			const internal::STypeCommon *stypeCommon = *iterMem;
			totalSize += stypeCommon->serialize(payload);
		}
		// DOCUMENT FOOTER : END
		payload.push_back(0);
		payload[offset + 0] = ((unsigned char)(totalSize >> 0));
		payload[offset + 1] = ((unsigned char)(totalSize >> 8));
		payload[offset + 2] = ((unsigned char)(totalSize >> 16));
		payload[offset + 3] = ((unsigned char)(totalSize >> 24));
		return payload.size() - offset;
	}

	size_t Serializable::deserialize(const std::vector<unsigned char>& payload, size_t offset) throw (ParseException)
	{
		uint32_t tempOffset = offset;
		internal::BsonParser parser(payload, payload.size(), &tempOffset);
		return parser.parse(this);
	}

	void Serializable::bsonParseHandle(uint8_t type, const std::string &name, const std::vector<unsigned char>& payload, uint32_t *offset, uint32_t docEndPos)
	{
		for (std::list<internal::STypeCommon*>::const_iterator iterMem = m_members.begin(); iterMem != m_members.end(); iterMem++)
		{
			internal::STypeCommon *stypeCommon = (*iterMem);
			if (stypeCommon->getMemberName() == name)
			{
				stypeCommon->deserialize(type, payload, offset, docEndPos);
				break;
			}
		}
	}

	uint32_t internal::BsonParser::parse(BsonParseHandler *handler)
	{
		docSize = readValue<uint32_t>(payload, offset, rootDocSize);
		docEndPos = *offset + docSize - 4;

		while ((docEndPos - *offset) > 0)
		{
			uint8_t type = payload[(*offset)++];
			if (type == 0)
				break;
			std::string ename;
			while(1)
			{
				char c;
				if ((docEndPos - *offset) == 0)
					throw Serializable::ParseException();
				c = payload[(*offset)++];
				if (!c)
					break;
				ename.push_back(c);
			}
			handler->bsonParseHandle(type, ename, payload, offset, docEndPos);
		}
		if((docEndPos - *offset) != 0)
			throw Serializable::ParseException();
		return docSize;
	}
}
