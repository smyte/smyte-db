#ifndef INFRA_AVROHELPER_H_
#define INFRA_AVROHELPER_H_

#include <memory>
#include <sstream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "avro/Encoder.hh"
#include "avro/Decoder.hh"
#include "avro/Specific.hh"
#include "avro/Stream.hh"
#pragma GCC diagnostic pop

namespace infra {

class AvroHelper {
 public:
  // Decode an avro record
  template <typename T>
  static void decode(const void* payload, size_t size, T* record) {
    auto input = std::unique_ptr<avro::InputStream>(
        avro::memoryInputStream(static_cast<const uint8_t*>(payload), size).release());
    avro::DecoderPtr decoder = avro::binaryDecoder();
    decoder->init(*input);
    avro::decode(*decoder, *record);
  }

  // Encode an avro record
  template <typename T>
  static void encode(const T& record, std::stringstream* ss) {
    avro::EncoderPtr e = avro::binaryEncoder();
    auto out = std::unique_ptr<avro::OutputStream>(avro::ostreamOutputStream(*ss).release());
    e->init(*out);
    avro::encode(*e, record);
    e->flush();
    out->flush();
  }
};

}  // namespace infra

#endif  // INFRA_AVROHELPER_H_
