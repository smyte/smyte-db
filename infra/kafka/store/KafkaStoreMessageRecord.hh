/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef INFRA_KAFKA_STORE_KAFKASTOREMESSAGERECORD_HH_441459953__H_
#define INFRA_KAFKA_STORE_KAFKASTOREMESSAGERECORD_HH_441459953__H_


#include <sstream>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "boost/any.hpp"
#include "avro/Specific.hh"
#include "avro/Encoder.hh"
#include "avro/Decoder.hh"
#pragma GCC diagnostic pop

namespace infra { namespace kafka { namespace store {
struct _KafkaStoreMessageRecord_json_Union__0__ {
private:
    size_t idx_;
    boost::any value_;
public:
    size_t idx() const { return idx_; }
    bool is_null() const {
        return (idx_ == 0);
    }
    void set_null() {
        idx_ = 0;
        value_ = boost::any();
    }
    std::vector<uint8_t> get_bytes() const;
    void set_bytes(const std::vector<uint8_t>& v);
    _KafkaStoreMessageRecord_json_Union__0__();
};

struct _KafkaStoreMessageRecord_json_Union__1__ {
private:
    size_t idx_;
    boost::any value_;
public:
    size_t idx() const { return idx_; }
    bool is_null() const {
        return (idx_ == 0);
    }
    void set_null() {
        idx_ = 0;
        value_ = boost::any();
    }
    std::vector<uint8_t> get_bytes() const;
    void set_bytes(const std::vector<uint8_t>& v);
    _KafkaStoreMessageRecord_json_Union__1__();
};

struct KafkaStoreMessage {
    typedef _KafkaStoreMessageRecord_json_Union__0__ key_t;
    typedef _KafkaStoreMessageRecord_json_Union__1__ value_t;
    int64_t timestamp;
    key_t key;
    value_t value;
    KafkaStoreMessage() :
        timestamp(int64_t()),
        key(key_t()),
        value(value_t())
        { }
};

inline
std::vector<uint8_t> _KafkaStoreMessageRecord_json_Union__0__::get_bytes() const {
    if (idx_ != 1) {
        throw avro::Exception("Invalid type for union");
    }
    return boost::any_cast<std::vector<uint8_t> >(value_);
}

inline
void _KafkaStoreMessageRecord_json_Union__0__::set_bytes(const std::vector<uint8_t>& v) {
    idx_ = 1;
    value_ = v;
}

inline
std::vector<uint8_t> _KafkaStoreMessageRecord_json_Union__1__::get_bytes() const {
    if (idx_ != 1) {
        throw avro::Exception("Invalid type for union");
    }
    return boost::any_cast<std::vector<uint8_t> >(value_);
}

inline
void _KafkaStoreMessageRecord_json_Union__1__::set_bytes(const std::vector<uint8_t>& v) {
    idx_ = 1;
    value_ = v;
}

inline _KafkaStoreMessageRecord_json_Union__0__::_KafkaStoreMessageRecord_json_Union__0__() : idx_(0) { }
inline _KafkaStoreMessageRecord_json_Union__1__::_KafkaStoreMessageRecord_json_Union__1__() : idx_(0) { }
}}}
namespace avro {
template<> struct codec_traits<infra::kafka::store::_KafkaStoreMessageRecord_json_Union__0__> {
    static void encode(Encoder& e, infra::kafka::store::_KafkaStoreMessageRecord_json_Union__0__ v) {
        e.encodeUnionIndex(v.idx());
        switch (v.idx()) {
        case 0:
            e.encodeNull();
            break;
        case 1:
            avro::encode(e, v.get_bytes());
            break;
        }
    }
    static void decode(Decoder& d, infra::kafka::store::_KafkaStoreMessageRecord_json_Union__0__& v) {
        size_t n = d.decodeUnionIndex();
        if (n >= 2) { throw avro::Exception("Union index too big"); }
        switch (n) {
        case 0:
            d.decodeNull();
            v.set_null();
            break;
        case 1:
            {
                std::vector<uint8_t> vv;
                avro::decode(d, vv);
                v.set_bytes(vv);
            }
            break;
        }
    }
};

template<> struct codec_traits<infra::kafka::store::_KafkaStoreMessageRecord_json_Union__1__> {
    static void encode(Encoder& e, infra::kafka::store::_KafkaStoreMessageRecord_json_Union__1__ v) {
        e.encodeUnionIndex(v.idx());
        switch (v.idx()) {
        case 0:
            e.encodeNull();
            break;
        case 1:
            avro::encode(e, v.get_bytes());
            break;
        }
    }
    static void decode(Decoder& d, infra::kafka::store::_KafkaStoreMessageRecord_json_Union__1__& v) {
        size_t n = d.decodeUnionIndex();
        if (n >= 2) { throw avro::Exception("Union index too big"); }
        switch (n) {
        case 0:
            d.decodeNull();
            v.set_null();
            break;
        case 1:
            {
                std::vector<uint8_t> vv;
                avro::decode(d, vv);
                v.set_bytes(vv);
            }
            break;
        }
    }
};

template<> struct codec_traits<infra::kafka::store::KafkaStoreMessage> {
    static void encode(Encoder& e, const infra::kafka::store::KafkaStoreMessage& v) {
        avro::encode(e, v.timestamp);
        avro::encode(e, v.key);
        avro::encode(e, v.value);
    }
    static void decode(Decoder& d, infra::kafka::store::KafkaStoreMessage& v) {
        if (avro::ResolvingDecoder *rd =
            dynamic_cast<avro::ResolvingDecoder *>(&d)) {
            const std::vector<size_t> fo = rd->fieldOrder();
            for (std::vector<size_t>::const_iterator it = fo.begin();
                it != fo.end(); ++it) {
                switch (*it) {
                case 0:
                    avro::decode(d, v.timestamp);
                    break;
                case 1:
                    avro::decode(d, v.key);
                    break;
                case 2:
                    avro::decode(d, v.value);
                    break;
                default:
                    break;
                }
            }
        } else {
            avro::decode(d, v.timestamp);
            avro::decode(d, v.key);
            avro::decode(d, v.value);
        }
    }
};

}
#endif
