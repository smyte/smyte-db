#ifndef INFRA_SERIALIZER_SERIALIZER_H_
#define INFRA_SERIALIZER_SERIALIZER_H_

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "boost/endian/buffers.hpp"
#include "glog/logging.h"

namespace serializer {

template <typename T>
class Serializer;

class Archive {
 public:
  Archive() : buf_(&mybuf_) {}  // writer

  explicit Archive(std::string* buf) : buf_(buf) {}  // writer

  explicit Archive(const std::string& s) : data_(s.data()), size_(s.size()), success_(true) {}  // reader

  Archive(const char* data, size_t size) : data_(data), size_(size), success_(true) {}  // reader

  std::string& buf() { return *buf_; }

  void append(const char* ptr, size_t size) { buf_->append(ptr, size); }

  bool success() const noexcept { return success_; }

  template <typename T>
  void put(const T& object) {
    Serializer<T>::serialize(object, this);
  }

  template <typename T>
  T get() {
    return Serializer<T>::deserialize(this);
  }

  const char* consume(size_t size) {
    if (success_ && size_ >= size) {
      auto ret = data_;
      data_ += size;
      size_ -= size;
      return ret;
    } else {
      success_ = false;
      LOG(ERROR) << "deserialization failure";
      return nullptr;
    }
  }

 private:
  const char* data_;
  size_t size_;
  std::string mybuf_;
  std::string* buf_;

  bool success_ = true;
};

template <typename T>
class Serializer {
 public:
  static T deserialize(Archive* archive);

  static void serialize(const T& value, Archive* archive);
};

template <typename T>
class BitSerializer {
 public:
  static T deserialize(Archive* archive) {
    auto data = archive->consume(sizeof(T));
    if (data != nullptr) {
      return *reinterpret_cast<const T*>(data);
    } else {
      T empty{};
      return empty;
    }
  }

  static void serialize(const T& value, Archive* archive) {
    archive->append(reinterpret_cast<const char*>(&value), sizeof(T));
  }
};

template <typename T>
Archive& operator>>(Archive& in, T& out) {
  out = serializer::Serializer<T>::deserialize(&in);
  return in;
}

template <typename T>
Archive& operator<<(Archive& out, const T& in) {
  serializer::Serializer<T>::serialize(in, &out);
  return out;
}

template <>
class Serializer<uint8_t> : public BitSerializer<uint8_t> {};

template <typename T>
class Serializer<std::vector<T>> {
 public:
  static std::vector<T> deserialize(Archive* archive) {
    auto size = archive->get<uint16_t>();
    std::vector<T> distances;
    auto len = sizeof(T) * size;
    auto data = archive->consume(len);
    if (data != nullptr) {
      distances.resize(size);
      memcpy(distances.data(), data, len);
    }
    return distances;
  }

  // this only works with vectors of 2^16 -1 or fewer items!!!
  static void serialize(const std::vector<T>& distances, Archive* archive) {
    auto size = distances.size();
    CHECK_LE(size, std::numeric_limits<uint16_t>::max());
    archive->put<uint16_t>(distances.size());
    archive->append(reinterpret_cast<const char*>(distances.data()), sizeof(T) * distances.size());
  }
};

template <>
class Serializer<uint16_t> {
 public:
  static uint16_t deserialize(Archive* archive) {
    auto data = archive->consume(sizeof(uint16_t));
    if (data != nullptr) {
      return boost::endian::detail::load_big_endian<uint16_t, sizeof(uint16_t)>(data);
    } else {
      return 0;
    }
  }

  static void serialize(const uint16_t& itemId, Archive* archive) {
    boost::endian::big_int16_buf_t value(itemId);
    archive->append(value.data(), sizeof(uint16_t));
  }
};

template <>
class Serializer<float> : public BitSerializer<float> {};

template <>
class Serializer<double> : public BitSerializer<double> {};

template <>
class Serializer<int32_t> {
 public:
  static int64_t deserialize(Archive* archive) {
    auto data = archive->consume(sizeof(int32_t));
    if (data != nullptr) {
      auto ret = boost::endian::detail::load_big_endian<int32_t, sizeof(int32_t)>(data);
      return ret;
    } else {
      return 0;
    }
  }

  static void serialize(const int64_t& itemId, Archive* archive) {
    boost::endian::big_int64_buf_t value(itemId);
    archive->append(value.data(), sizeof(int64_t));
  }
};

template <>
class Serializer<int64_t> {
 public:
  static int64_t deserialize(Archive* archive) {
    auto data = archive->consume(sizeof(int64_t));
    if (data != nullptr) {
      auto ret = boost::endian::detail::load_big_endian<int64_t, sizeof(int64_t)>(data);
      return ret;
    } else {
      return 0;
    }
  }

  static void serialize(const int64_t& itemId, Archive* archive) {
    boost::endian::big_int64_buf_t value(itemId);
    archive->append(value.data(), sizeof(int64_t));
  }
};

template <>
class Serializer<uint64_t> {
 public:
  static uint64_t deserialize(Archive* archive) {
    auto data = archive->consume(sizeof(uint64_t));
    if (data != nullptr) {
      return boost::endian::detail::load_big_endian<uint64_t, sizeof(uint64_t)>(data);
    } else {
      return 0;
    }
  }

  static void serialize(const uint64_t& itemId, Archive* archive) {
    boost::endian::big_int64_buf_t value(itemId);
    archive->append(value.data(), sizeof(uint64_t));
  }
};

template <>
class Serializer<std::string> {
 public:
  static std::string deserialize(Archive* archive) {
    auto len = archive->get<uint16_t>();
    std::string buf;
    auto data = archive->consume(len);
    if (data != nullptr) {
      buf.append(data, len);
    }
    return buf;
  }

  // this only works with strings of 2^16 -1 or fewer chars!!!
  static void serialize(const std::string& value, Archive* archive) {
    auto size = value.size();
    CHECK_LE(size, std::numeric_limits<uint16_t>::max());
    archive->put(static_cast<uint16_t>(value.size()));
    archive->append(value.data(), value.size());
  }
};

template <typename T>
static std::string asString(const T& value) {
  Archive archive;
  Serializer<T>::serialize(value, &archive);
  return archive.buf();
}

}  // namespace serializer

#endif  // INFRA_SERIALIZER_SERIALIZER_H_
