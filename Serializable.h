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
 * @file	Serializable.h
 * @author	Jichan (development@jc-lab.net / http://ablog.jc-lab.net/ )
 * @date	2019/04/10
 * @copyright Copyright (C) 2018 jichan.\n
 *            This software may be modified and distributed under the terms
 *            of the Apache License 2.0.  See the LICENSE file for details.
 */
#pragma once

#include <stdint.h>
#include <wchar.h>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <exception>

#include <assert.h>

#if defined(HAS_JSCPPUTILS) && HAS_JSCPPUTILS
#include <JsCPPUtils/SmartPointer.h>
#include <JsCPPUtils/Base64.h>
#endif

namespace JsBsonRPC {

	class DeserializationConfig {
	public:
		struct BuildContext {
			int ordinal;
			std::list<DeserializationConfig> list;
		};
	private:
		bool defaultValue;
		uint32_t mask;

		static BuildContext *getBuildContext();

	public:
		DeserializationConfig(bool defaultValue);
		uint32_t getMask() const { return mask; }
		bool getDefaultValue() const { return defaultValue; }
		
	public:
		static uint32_t getDefaultConfigure();
		static DeserializationConfig FAIL_ON_UNKNOWN_PROPERTIES;
	};

	class Serializable;

	class SerializableCreateFactory
	{
	public:
		virtual Serializable *create() = 0;
	};

	class SerializableSmartpointerCreateFactory;
#if defined(HAS_JSCPPUTILS) && HAS_JSCPPUTILS
	class SerializableSmartpointerCreateFactory
	{
	public:
		virtual JsCPPUtils::SmartPointer<Serializable> create() = 0;
		virtual JsCPPUtils::SmartPointer<Serializable> create(const std::string& name, int64_t serialVersionUID) { return NULL; }
	};
#endif

	namespace internal {
		class STypeCommon
		{
		protected:
			std::string key;
			SerializableCreateFactory *createFactory;
			SerializableSmartpointerCreateFactory *createSmartpointerFactory;
			bool _isnull;

		public:
			STypeCommon() {
				this->createFactory = NULL;
				this->createSmartpointerFactory = NULL;
			}
			virtual ~STypeCommon() {}

			STypeCommon &setCreateFactory(SerializableCreateFactory *factory) {
				this->createFactory = factory;
				return *this;
			}

			STypeCommon &setCreateFactory(SerializableSmartpointerCreateFactory *factory) {
				this->createSmartpointerFactory = factory;
				return *this;
			}

			SerializableCreateFactory *getSerializableCreateFactory() {
				return this->createFactory;
			}

			SerializableSmartpointerCreateFactory *getSerializableSmartpointerCreateFactory() {
				return this->createSmartpointerFactory;
			}
			
			void setMemberName(const char *name) {
				if (key.empty())
					key = name;
			}

			std::string getMemberName() const {
				return key;
			}

			void setNull() {
				_isnull = true;
			}

			bool isNull() const {
				return _isnull;
			}

			virtual void clear() = 0;
			virtual uint32_t serialize(std::vector<unsigned char> &payload) const = 0;
			virtual uint32_t deserialize(uint8_t type, const std::vector<unsigned char> &payload, uint32_t *offset, uint32_t documentSize) = 0;
		};

		class BsonParseHandler {
		public:
			virtual void serializableNameHandle(const std::string& attrName, const std::string& value) {}
			virtual void serializableSerialVersionUIDHandle(const std::string& attrName, int64_t value) {}
			virtual bool bsonParseHandle(uint8_t type, const std::string &name, const std::vector<unsigned char>& payload, uint32_t *offset, uint32_t docEndPos) = 0;
		};
	}

	template<typename T>
	class SType : public internal::STypeCommon
	{
	private:
		T object;

	public:
		uint32_t serialize(std::vector<unsigned char> &payload) const override
		{
			return internal::ObjectHelper<internal::IsSerializableClass<T>::Result, T>::serialize(payload, this->key, this->object);
		}

		uint32_t deserialize(uint8_t type, const std::vector<unsigned char> &payload, uint32_t *offset, uint32_t documentSize) override
		{
			if (type == internal::BSONTYPE_NULL) {
				clear();
				setNull();
				return 0;
			}
			return internal::ObjectHelper<internal::IsSerializableClass<T>::Result, T>::deserialize(this, object, type, payload, offset, documentSize);
		}

		void clear() override {
			internal::ObjectHelper<internal::IsSerializableClass<T>::Result, T>::objectClear(this->object);
		}

		SType<T> &operator=(const T& value) {
			this->_isnull = false;
			this->object = value;
			return *this;
		}

		const T& get() const {
			return this->object;
		}

		T& ref() {
			this->_isnull = true;
			return this->object;
		}

		void set(const T& value) {
			this->object = value;
		}
	};

	class Serializable : protected internal::BsonParseHandler
	{
	public:
		class UnavailableTypeException : public std::exception
		{ };
		class ParseException : public std::exception
		{ };

	private:
		static const unsigned char header[];
		std::string m_name;
		int64_t m_serialVersionUID;
		std::list<internal::STypeCommon*> m_members;

		uint32_t m_deserializationConfigs;

	protected:
#if (__cplusplus >= 201103) || (__cplusplus == 199711) || (defined(HAS_MOVE_SEMANTICS) && HAS_MOVE_SEMANTICS == 1)
		explicit Serializable(Serializable&& _ref)
		{
			assert(false);
		}
#endif
		bool bsonParseHandle(uint8_t type, const std::string &name, const std::vector<unsigned char>& payload, uint32_t *offset, uint32_t docEndPos) override;

	private:
		Serializable(const Serializable &obj) {
			assert(false);
		}

	public:
		Serializable(const char *name, int64_t serialVersionUID);
		virtual ~Serializable();

		Serializable& operator=(const Serializable& obj) {
			assert(this->m_name == obj.m_name);
			assert(this->m_serialVersionUID == obj.m_serialVersionUID);
			std::vector<uint8_t> payload;
			this->m_deserializationConfigs = obj.m_deserializationConfigs;
			obj.serialize(payload);
			this->deserialize(payload);
			return *this;
		}

		const std::list<internal::STypeCommon*> &serializableMembers() const { return m_members; }

		size_t serialize(std::vector<unsigned char>& payload) const throw(UnavailableTypeException);
		size_t deserialize(const std::vector<unsigned char>& payload, size_t offset = 0) throw (ParseException);

		void serializableClearObjects();

		std::string serializableGetName() {
			return m_name;
		}
		int64_t serializableGetSerialVersionUID() {
			return m_serialVersionUID;
		}

		void serializableConfigure(const DeserializationConfig &deserializationConfig, bool enable);

		static bool readMetadata(const std::vector<unsigned char>& payload, size_t offset, std::string *pName = NULL, int64_t *pSerialVersionUID = NULL, uint32_t *pDocSize = NULL);

	protected:
		internal::STypeCommon &serializableMapMember(const char *name, internal::STypeCommon &object);

	private:
		bool checkFlagsAll(int value, int type) const
		{
			return (value & type) == type;
		}
	};

	namespace internal {
		enum BsonTypes {
			BSONTYPE_DOUBLE = 0x01,
			BSONTYPE_STRING_UTF8 = 0x02,
			BSONTYPE_DOCUMENT = 0x03,
			BSONTYPE_ARRAY = 0x04,
			BSONTYPE_BINARY = 0x05,
			BSONTYPE_OBJECTID = 0x07,
			BSONTYPE_BOOL = 0x08,
			BSONTYPE_UTCDATETIME = 0x09,
			BSONTYPE_NULL = 0x0A,
			BSONTYPE_INT32 = 0x10,
			BSONTYPE_TIMESTAMP = 0x11,
			BSONTYPE_INT64 = 0x12,
			BSONTYPE_DECIMAL128 = 0x13,
		};

		extern int _my_itoa(
			int    value,
			char*  buf, \
			size_t bufSize,
			int    radix
		);

		extern uint32_t serializeKey(std::vector<unsigned char> &payload, const std::string& key);

		extern uint32_t serializeNullObject(std::vector<unsigned char> &payload, const std::string& key);

		template <typename T>
		T readValue(const std::vector<unsigned char> &payload, uint32_t *offset, uint32_t documentSize) {
			int i;
			T value;
			unsigned char *carr = (unsigned char*)&value;
			if ((documentSize - *offset) < sizeof(value))
				throw Serializable::ParseException();
			for (i = 0; i < sizeof(value); i++)
				carr[i] = (unsigned char)payload[(*offset)++];
			return value;
		}

		class BsonParser {
		private:
			uint32_t deserializationConfigs;

			const std::vector<unsigned char> &payload;
			uint32_t docSize;
			uint32_t rootDocSize;
			uint32_t *offset;
			uint32_t docEndPos;

		public:
			BsonParser(const std::vector<unsigned char> &_payload, uint32_t rootDocSize, uint32_t *rootDocOffset, uint32_t deserializationConfigs) : payload(_payload)
			{
				this->deserializationConfigs = deserializationConfigs;
				this->docSize = 0;
				this->docEndPos = 0;
				this->rootDocSize = rootDocSize;
				this->offset = rootDocOffset;
			}

			uint32_t parse(BsonParseHandler *handler);
		};

		template<typename T>
		class IsSerializableSmartpointerClass
		{
		public:
			enum { Result = 0 };
		};

#if defined(HAS_JSCPPUTILS) && HAS_JSCPPUTILS
		template<typename D>
		class IsSerializableSmartpointerClass< JsCPPUtils::SmartPointer<D> >
		{
			class No { };
			class Yes { No no[3]; };

			static Yes Test(Serializable*); // not defined
			static No Test(...); // not defined 

			static void Constraints(D* p) { Serializable* pb = p; pb = p; }

		public:
			enum { Result = (sizeof(Test(static_cast<D*>(0))) == sizeof(Yes)) ? 1 : 0 };

			IsSerializableSmartpointerClass() { void(*p)(D*) = Constraints; }
		};
#endif

		template<typename D>
		class IsSerializableClass
		{
			class No { };
			class Yes { No no[3]; };

			static Yes Test(Serializable*); // not defined
			static No Test(...); // not defined 

			static void Constraints(D* p) { Serializable* pb = p; pb = p; }

		public:
			enum { Result = (sizeof(Test(static_cast<D*>(0))) == sizeof(Yes)) ? 1 : IsSerializableSmartpointerClass<D>::Result };

			IsSerializableClass() { void(*p)(D*) = Constraints; }
		};

		template<int PreType, typename T>
		struct ObjectHelper {
		};

#define _JSBSONRPC_MAKESERIALIZE_BASICTYPE(TYPE, BSONTYPE) \
		template<> \
		struct ObjectHelper<0, TYPE> { \
			static uint32_t serialize(std::vector<unsigned char> &payload, const std::string& key, const TYPE &object) { \
				uint32_t payloadLen = 1 + sizeof(TYPE); \
				int i; \
				const unsigned char *carr = (const unsigned char*)&object; \
				payload.push_back(BSONTYPE); \
				payloadLen += serializeKey(payload, key); \
				for(i=0; i<sizeof(TYPE); i++, carr++) \
					payload.push_back(*carr); \
				return payloadLen; \
			} \
			static uint32_t deserialize(internal::STypeCommon *rootSType, TYPE &object, uint8_t type, const std::vector<unsigned char> &payload, uint32_t *offset, uint32_t documentSize) { \
				if(type == BSONTYPE_INT32) { \
					object = readValue<int32_t>(payload, offset, documentSize); \
					return sizeof(int32_t); \
				}else if(type == BSONTYPE_INT64) { \
					object = readValue<int64_t>(payload, offset, documentSize); \
					return sizeof(int64_t); \
				}else if(type == BSONTYPE_BOOL) { \
					object = readValue<unsigned char>(payload, offset, documentSize); \
					return 1; \
				} else if(type == BSONTYPE_DOUBLE) { \
					object = readValue<double>(payload, offset, documentSize); \
					return sizeof(double); \
				} else if((type == BSONTYPE_UTCDATETIME) || (type == BSONTYPE_TIMESTAMP)) { \
					object = readValue<uint64_t>(payload, offset, documentSize); \
					return sizeof(uint64_t); \
				} else if(type == BSONTYPE_NULL) { return 0; } \
				throw Serializable::ParseException(); \
			} \
			static void objectClear(TYPE &object) { object = 0; } \
		};

#define _JSBSONRPC_MAKESERIALIZE_BASICSMALLTYPE(TYPE, SERTYPE, INT32TYPE, INT64TYPE, BSONTYPE) \
		template<> \
		struct ObjectHelper<0, TYPE> { \
			static uint32_t serialize(std::vector<unsigned char> &payload, const std::string& key, const TYPE &object) { \
				uint32_t payloadLen = 1 + sizeof(SERTYPE); \
				int i; \
				SERTYPE serValue = object; \
				const unsigned char *carr = (const unsigned char*)&serValue; \
				payload.push_back(BSONTYPE); \
				payloadLen += serializeKey(payload, key); \
				for(i=0; i<sizeof(serValue); i++, carr++) \
					payload.push_back(*carr); \
				return payloadLen; \
			} \
			static uint32_t deserialize(internal::STypeCommon *rootSType, TYPE &object, uint8_t type, const std::vector<unsigned char> &payload, uint32_t *offset, uint32_t documentSize) { \
				if(type == BSONTYPE_INT32) { \
					object = readValue<INT32TYPE>(payload, offset, documentSize); \
					return sizeof(int32_t); \
				}else if(type == BSONTYPE_INT64) { \
					object = readValue<INT64TYPE>(payload, offset, documentSize); \
					return sizeof(int64_t); \
				}else if(type == BSONTYPE_BOOL) { \
					object = readValue<unsigned char>(payload, offset, documentSize); \
					return 1; \
				} else if(type == BSONTYPE_DOUBLE) { \
					object = readValue<double>(payload, offset, documentSize); \
					return sizeof(double); \
				} else if((type == BSONTYPE_UTCDATETIME) || (type == BSONTYPE_TIMESTAMP)) { \
					object = readValue<uint64_t>(payload, offset, documentSize); \
					return sizeof(uint64_t); \
				} else if(type == BSONTYPE_NULL) { return 0; } \
				throw Serializable::ParseException(); \
			} \
			static void objectClear(TYPE &object) { object = 0; } \
		};

		_JSBSONRPC_MAKESERIALIZE_BASICTYPE(int32_t, BSONTYPE_INT32);
		_JSBSONRPC_MAKESERIALIZE_BASICTYPE(uint32_t, BSONTYPE_INT32);
		_JSBSONRPC_MAKESERIALIZE_BASICTYPE(int64_t, BSONTYPE_INT64);
		_JSBSONRPC_MAKESERIALIZE_BASICTYPE(uint64_t, BSONTYPE_INT64);
		_JSBSONRPC_MAKESERIALIZE_BASICTYPE(double, BSONTYPE_DOUBLE);
		_JSBSONRPC_MAKESERIALIZE_BASICSMALLTYPE(int8_t, int32_t, int32_t, int64_t, BSONTYPE_INT32)
		_JSBSONRPC_MAKESERIALIZE_BASICSMALLTYPE(uint8_t, uint32_t, uint32_t, uint64_t, BSONTYPE_INT32)
		_JSBSONRPC_MAKESERIALIZE_BASICSMALLTYPE(int16_t, int32_t, int32_t, int64_t, BSONTYPE_INT32)
		_JSBSONRPC_MAKESERIALIZE_BASICSMALLTYPE(uint16_t, uint32_t, uint32_t, uint64_t, BSONTYPE_INT32)

		template<>
		struct ObjectHelper<0, float> {
			static uint32_t serialize(std::vector<unsigned char> &payload, const std::string& key, const float &object) {
				uint32_t payloadLen = 1 + sizeof(double);
				int i;
				double dblValue = object;
				const unsigned char *carr = (const unsigned char*)&object;
				payload.push_back(BSONTYPE_DOUBLE);
				payloadLen += serializeKey(payload, key);
				for (i = 0; i<sizeof(dblValue); i++, carr++)
					payload.push_back(*carr);
				return payloadLen;
			}
			static uint32_t deserialize(internal::STypeCommon *rootSType, float &object, uint8_t type, const std::vector<unsigned char> &payload, uint32_t *offset, uint32_t documentSize) {
				if (type == BSONTYPE_INT32) {
					object = readValue<int32_t>(payload, offset, documentSize);
					return sizeof(int32_t);
				}
				else if (type == BSONTYPE_INT64) {
					object = readValue<int64_t>(payload, offset, documentSize);
					return sizeof(int64_t);
				}
				else if (type == BSONTYPE_BOOL) {
					object = readValue<unsigned char>(payload, offset, documentSize);
					return 1;
				}
				else if (type == BSONTYPE_DOUBLE) {
					object = readValue<double>(payload, offset, documentSize);
					return sizeof(double);
				}
				else if ((type == BSONTYPE_UTCDATETIME) || (type == BSONTYPE_TIMESTAMP)) {
					object = readValue<uint64_t>(payload, offset, documentSize);
					return sizeof(uint64_t);
				}
				else if (type == BSONTYPE_NULL) { return 0; }
				throw Serializable::ParseException();
			}
			static void objectClear(float &object) { object = 0; }
		};


		template<>
		struct ObjectHelper<0, bool> {
			static uint32_t serialize(std::vector<unsigned char> &payload, const std::string& key, const bool &object) {
				uint32_t payloadLen = 2;
				payload.push_back(internal::BSONTYPE_BOOL);
				payloadLen += serializeKey(payload, key);
				payload.push_back(object ? 1 : 0);
				return payloadLen;
			}
			static uint32_t deserialize(internal::STypeCommon *rootSType, bool &object, uint8_t type, const std::vector<unsigned char> &payload, uint32_t *offset, uint32_t documentSize) {
				if (type == BSONTYPE_BOOL) {
					object = readValue<unsigned char>(payload, offset, documentSize) ? true : false;
					return 1;
				}else if (type == BSONTYPE_INT32) {
					object = readValue<int32_t>(payload, offset, documentSize) ? true : false;
					return sizeof(int32_t);
				} else if (type == BSONTYPE_INT64) {
					object = readValue<int64_t>(payload, offset, documentSize) ? true : false;
					return sizeof(int64_t);
				} else if (type == BSONTYPE_NULL) { return 0; }
				throw Serializable::ParseException();
			}
			static void objectClear(bool &object) {
				object = false;
			}
		};

		template<>
		struct ObjectHelper<0, std::string> {
			static uint32_t serialize(std::vector<unsigned char> &payload, const std::string& key, const std::string &object) {
				uint32_t len = object.length() + 1;
				uint32_t payloadLen = 5 + len;
				payload.push_back(internal::BSONTYPE_STRING_UTF8);
				payloadLen += serializeKey(payload, key);
				payload.push_back((unsigned char)(len >> 0));
				payload.push_back((unsigned char)(len >> 8));
				payload.push_back((unsigned char)(len >> 16));
				payload.push_back((unsigned char)(len >> 24));
				payload.insert(payload.end(), object.begin(), object.end());
				payload.push_back(0);
				return payloadLen;
			}
			static uint32_t deserialize(internal::STypeCommon *rootSType, std::string &object, uint8_t type, const std::vector<unsigned char> &payload, uint32_t *offset, uint32_t documentSize) {
				uint32_t payloadSize = 0;
				if (type == BSONTYPE_STRING_UTF8) {
					uint32_t len = readValue<uint32_t>(payload, offset, documentSize);
					if ((documentSize - *offset) < len)
						throw Serializable::ParseException();
					if(payload[*offset + len - 1] == 0)
						object = std::string((const char*)&payload[*offset], len - 1);
					else
						object = std::string((const char*)&payload[*offset], len);
					*offset += len;
					payloadSize = 4 + len;
				} else {
					throw Serializable::ParseException();
				}
				return payloadSize;
			}
			static void objectClear(std::string &object) {
				object.clear();
			}
		};

		template<typename T>
		struct ObjectHelper< 0, std::vector<T> > {
			static uint32_t serialize(std::vector<unsigned char> &payload, const std::string& key, const std::vector<T> &object) {
				size_t i, len = object.size();
				uint32_t totallen = len * sizeof(T);
				uint32_t payloadLen = totallen + 6;
				payload.push_back(internal::BSONTYPE_BINARY);
				payloadLen += serializeKey(payload, key);
				payload.push_back((unsigned char)(len >> 0));
				payload.push_back((unsigned char)(len >> 8));
				payload.push_back((unsigned char)(len >> 16));
				payload.push_back((unsigned char)(len >> 24));
				payload.push_back(0x00); // Generic binary subtype
				for (i = 0; i < len; i++)
				{
					int j;
					const unsigned char *carr = (const unsigned char*)&object[i];
					for (j = 0; j < sizeof(T); j++)
						payload.push_back(carr[j]);
				}
				return payloadLen;
			}
			static uint32_t deserialize(internal::STypeCommon *rootSType, std::vector<T> &object, uint8_t type, const std::vector<unsigned char> &payload, uint32_t *offset, uint32_t documentSize) {
				uint32_t payloadSize = 0;
				object.clear();
				if (type == BSONTYPE_BINARY) {
					uint32_t len = readValue<uint32_t>(payload, offset, documentSize);
					uint8_t subtype = readValue<uint8_t>(payload, offset, documentSize);
					uint32_t cnt = len / sizeof(T);
					uint32_t i;
					object.clear();
					payloadSize = 5 + cnt * sizeof(T);
					for (i = 0; i < cnt; i++) {
						object.push_back(readValue<T>(payload, offset, documentSize));
					}
#if defined(HAS_JSCPPUTILS) && HAS_JSCPPUTILS
				} else if (type == BSONTYPE_STRING_UTF8) {
					// BASE64
					std::vector<unsigned char> buffer;
					uint32_t len = readValue<uint32_t>(payload, offset, documentSize);
					uint32_t realLen = len;
					if (payload[*offset + len - 1] == 0)
						realLen--;
					JsCPPUtils::Base64::decode(buffer, (const char*)&payload[*offset], realLen);
					object.insert(object.end(), buffer.begin(), buffer.end());
					*offset += len;
					payloadSize += 4 + len;
#endif
				}else{
					throw Serializable::ParseException();
				}
				return payloadSize;
			}
			static void objectClear(std::vector<T> &object) {
				object.clear();
			}
		};

		template<typename T>
		struct ObjectHelper< 0, std::list<T> > : public BsonParseHandler {
			internal::STypeCommon *rootSType;
			std::list<T> &refObject;
			ObjectHelper(internal::STypeCommon *_rootSType, std::list<T> &object) : rootSType(_rootSType), refObject(object) {}

			static uint32_t serialize(std::vector<unsigned char> &payload, const std::string& key, const std::list<T> &object) {
				size_t offset;
				char seqKey[32];
				int i = 0;
				uint32_t payloadLen = 1;
				uint32_t subDocumentSize = 5;
				payload.push_back(internal::BSONTYPE_ARRAY);
				payloadLen += serializeKey(payload, key);
				offset = payload.size();
				// DOCUMENT HEADER : SIZE
				payload.push_back(0); payload.push_back(0); payload.push_back(0); payload.push_back(0);
				for (std::list<T>::const_iterator iter = object.begin(); iter != object.end(); iter++)
				{
					// doucment
					_my_itoa(i, seqKey, sizeof(seqKey), 10);
					subDocumentSize += ObjectHelper<internal::IsSerializableClass<T>::Result, T>::serialize(payload, seqKey, *iter);
					i++;
				}
				// DOCUMENT FOOTER : END
				payload.push_back(0);
				payload[offset + 0] = ((unsigned char)(subDocumentSize >> 0));
				payload[offset + 1] = ((unsigned char)(subDocumentSize >> 8));
				payload[offset + 2] = ((unsigned char)(subDocumentSize >> 16));
				payload[offset + 3] = ((unsigned char)(subDocumentSize >> 24));
				payloadLen += subDocumentSize;
				return payloadLen;
			}
			static uint32_t deserialize(internal::STypeCommon *rootSType, std::list<T> &object, uint8_t type, const std::vector<unsigned char> &payload, uint32_t *offset, uint32_t documentSize) {
				BsonParser parser(payload, documentSize, offset, DeserializationConfig::getDefaultConfigure());
				ObjectHelper< 0, std::list<T> > helper(rootSType, object);
				object.clear();
				return parser.parse(&helper);
			}
			static void objectClear(std::list<T> &object) {
				object.clear();
			}
			bool bsonParseHandle(uint8_t type, const std::string &name, const std::vector<unsigned char>& payload, uint32_t *offset, uint32_t docEndPos) override {
				T temp;
				ObjectHelper<internal::IsSerializableClass<T>::Result, T>::deserialize(rootSType, temp, type, payload, offset, docEndPos);
				refObject.push_back(temp);
				return true;
			};
		};

		template<typename T>
		struct ObjectHelper< 0, std::map<std::string, T> > : public BsonParseHandler {
			internal::STypeCommon *rootSType;
			std::map<std::string, T> &refObject;
			ObjectHelper(internal::STypeCommon *_rootSType, std::map<std::string, T> &object) : rootSType(_rootSType), refObject(object) {}

			static uint32_t serialize(std::vector<unsigned char> &payload, const std::string& key, const std::map<std::string, T> &object) {
				size_t offset;
				uint32_t payloadLen = 1;
				uint32_t subDocumentSize = 5;
				payload.push_back(internal::BSONTYPE_DOCUMENT);
				payloadLen += serializeKey(payload, key);
				offset = payload.size();

				// DOCUMENT HEADER : SIZE
				payload.push_back(0); payload.push_back(0); payload.push_back(0); payload.push_back(0);
				for (std::map<std::string, T>::const_iterator iter = object.begin(); iter != object.end(); iter++)
				{
					// doucment
					subDocumentSize += ObjectHelper<internal::IsSerializableClass<T>::Result, T>::serialize(payload, iter->first, iter->second);
				}
				// DOCUMENT FOOTER : END
				payload.push_back(0);
				payload[offset + 0] = ((unsigned char)(subDocumentSize >> 0));
				payload[offset + 1] = ((unsigned char)(subDocumentSize >> 8));
				payload[offset + 2] = ((unsigned char)(subDocumentSize >> 16));
				payload[offset + 3] = ((unsigned char)(subDocumentSize >> 24));
				payloadLen += subDocumentSize;
				return payloadLen;
			}
			static uint32_t deserialize(internal::STypeCommon *rootSType, std::map<std::string, T> &object, uint8_t type, const std::vector<unsigned char> &payload, uint32_t *offset, uint32_t documentSize) {
				BsonParser parser(payload, documentSize, offset, DeserializationConfig::getDefaultConfigure());
				ObjectHelper< 0, std::map<std::string, T> > helper(rootSType, object);
				object.clear();
				return parser.parse(&helper);
			}
			static void objectClear(std::map<std::string, T> &object) {
				object.clear();
			}
			bool bsonParseHandle(uint8_t type, const std::string &name, const std::vector<unsigned char>& payload, uint32_t *offset, uint32_t docEndPos) override {
				ObjectHelper<internal::IsSerializableClass<T>::Result, T>::deserialize(rootSType, refObject[name], type, payload, offset, docEndPos);
				return true;
			};
		};

		template<typename T>
		struct ObjectHelper<1, T> {
			static uint32_t serialize(std::vector<unsigned char> &payload, const std::string& key, const Serializable &object) {
				size_t payloadLen = 1;
				payload.push_back(internal::BSONTYPE_DOCUMENT);
				payloadLen += serializeKey(payload, key);
				payloadLen += object.serialize(payload);
				return payloadLen;
			}
			static uint32_t deserialize(internal::STypeCommon *rootSType, Serializable &object, uint8_t type, const std::vector<unsigned char> &payload, uint32_t *offset, uint32_t documentSize) {
				uint32_t payloadLen = object.deserialize(payload, *offset);
				*offset += payloadLen;
				return payloadLen;
			}
			static void objectClear(Serializable &object) {
				object.serializableClearObjects();
			}
		};

#if defined(HAS_JSCPPUTILS) && HAS_JSCPPUTILS
		template<typename T>
		struct ObjectHelper<1, JsCPPUtils::SmartPointer<T> > {
			static uint32_t serialize(std::vector<unsigned char> &payload, const std::string& key, const JsCPPUtils::SmartPointer<T> &object) {
				size_t payloadLen = 1;
				if (!object) {
					return serializeNullObject(payload, key);
				}
				payload.push_back(internal::BSONTYPE_DOCUMENT);
				payloadLen += serializeKey(payload, key);
				payloadLen += object->serialize(payload);
				return payloadLen;
			}
			static uint32_t deserialize(internal::STypeCommon *rootSType, JsCPPUtils::SmartPointer<T> &object, uint8_t type, const std::vector<unsigned char> &payload, uint32_t *offset, uint32_t documentSize) {
				if (rootSType->getSerializableSmartpointerCreateFactory())
				{
					std::string sname;
					int64_t sver;
					Serializable::readMetadata(payload, *offset, &sname, &sver);
					JsCPPUtils::SmartPointer<Serializable> newObj = rootSType->getSerializableSmartpointerCreateFactory()->create(sname, sver);
					if (!newObj)
						newObj = rootSType->getSerializableSmartpointerCreateFactory()->create();
					if (newObj != NULL) {
						object.attach(newObj.detach());
					}
				}
				
				uint32_t payloadLen = object->deserialize(payload, *offset);
				*offset += payloadLen;
				return payloadLen;
			}
			static void objectClear(JsCPPUtils::SmartPointer<T> &object) {
				object->serializableClearObjects();
			}
		};
#endif
	}
}
