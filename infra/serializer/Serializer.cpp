#include <string>

#include "infra/serializer/Serializer.h"

namespace serializer {

template class Serializer<uint8_t>;
template class Serializer<float>;
template class Serializer<double>;
template class Serializer<int32_t>;
template class Serializer<int64_t>;
template class Serializer<uint64_t>;
template class Serializer<std::string>;

}  // namespace serializer
