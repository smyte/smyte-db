#include "codec/RedisDecoder.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "folly/Conv.h"
#include "folly/io/Cursor.h"
#include "glog/logging.h"

namespace codec {

// Decode Redis Array of Bulk String into a RedisValue as result
bool RedisDecoder::decode(Context* ctx, folly::IOBufQueue& buf, RedisValue& result, size_t& needed) {
  if (buf.chainLength() < kMinBytesNeeded) {
    needed = kMinBytesNeeded - buf.chainLength();
    return false;
  }

  folly::io::Cursor start(buf.front());
  folly::io::Cursor curr(buf.front());
  // having noise before Array length field does not break the protocol, so skip them
  skipNoise(&curr);
  buf.trimStart(curr - start);
  if (buf.chainLength() == 0) {
    needed = kMinBytesNeeded;
    return false;
  }
  // reset the cursor after trimming
  start.reset(buf.front());
  curr.reset(buf.front());

  LengthFieldState arrayLengthState = LengthFieldState::kInvalid;
  int64_t arrayLength = readLength(RedisValue::kTypeIndicators[static_cast<int>(RedisValue::Type::kArray)],
                                   &curr, &arrayLengthState, &needed);
  if (arrayLengthState == LengthFieldState::kInvalid) {
    // encountered a protocol error, all bytes read so far will be abandoned
    result = RedisValue(RedisValue::Type::kError, "Protocol Error: Invalid Array length");
    buf.trimStart(curr - start);
    if (buf.chainLength() < kMinBytesNeeded) needed = kMinBytesNeeded - buf.chainLength();
    return true;
  } else if (arrayLengthState == LengthFieldState::kMoreBytesNeeded) {
    return false;
  }

  if (arrayLength <= 0) {
    // -1 means NULL array, 0 means empty array
    // both are valid values, but server does not know how to handle them
    if (arrayLength < -1) {
      LOG(WARNING) << "-1 is the only valid negative Array length";
    }
    result = RedisValue(RedisValue::Type::kError, "Protocol Error: Invalid Array length");
    buf.trimStart(curr - start);
    if (buf.chainLength() < kMinBytesNeeded) needed = kMinBytesNeeded - buf.chainLength();
    return true;
  }

  std::vector<std::string> strings;
  strings.reserve(arrayLength);
  for (int64_t i = 0; i < arrayLength; i++) {
    LengthFieldState stringLengthState = LengthFieldState::kInvalid;
    int64_t stringLength = readLength(RedisValue::kTypeIndicators[static_cast<int>(RedisValue::Type::kBulkString)],
                                      &curr, &stringLengthState, &needed);

    if (stringLengthState == LengthFieldState::kInvalid) {
      // encountered a protocol error, all bytes read so far will be abandoned
      result = RedisValue(RedisValue::Type::kError, "Protocol Error: Invalid Bulk String length");
      buf.trimStart(curr - start);
      if (buf.chainLength() < kMinBytesNeeded) needed = kMinBytesNeeded - buf.chainLength();
      return true;
    } else if (stringLengthState == LengthFieldState::kMoreBytesNeeded) {
      // protocol is still in good state, just wait for more bytes
      return false;
    }

    if (stringLength <= 0) {
      // -1 means NULL string, 0 means empty string
      // both are valid values, but server does not know how to handle them
      if (arrayLength < -1) {
        LOG(WARNING) << "-1 is the only valid negative Bulk String length";
      }
      result = RedisValue(RedisValue::Type::kError, "Protocol Error: Invalid Bulk String length");
      buf.trimStart(curr - start);
      if (buf.chainLength() < kMinBytesNeeded) needed = kMinBytesNeeded - buf.chainLength();
      return true;
    }

    if (curr.totalLength() < (static_cast<size_t>(stringLength) + 2)) {  // string + '\r\n'
      // no trimming here, we will start over from '*' once more bytes are available
      needed = stringLength + 2 - curr.totalLength();
      return false;
    }

    strings.emplace_back(curr.readFixedString(stringLength));

    // make sure this field terminates with '\r\n'
    if (curr.totalLength() < 2) {
      needed = 2;
      return false;
    } else if (curr.read<char>() != '\r' || curr.read<char>() != '\n') {
      result = RedisValue(RedisValue::Type::kError, "Protocol Error: Expect '\\r\\n'");
      buf.trimStart(curr - start);
      if (buf.chainLength() < kMinBytesNeeded) needed = kMinBytesNeeded - buf.chainLength();
      return true;
    }
  }

  result = RedisValue(std::move(strings));
  buf.trimStart(curr - start);
  if (buf.chainLength() < kMinBytesNeeded) needed = kMinBytesNeeded - buf.chainLength();
  return true;
}

// Decode the length field for both Arrays and Bulk Strings
int64_t RedisDecoder::readLength(char typeIndicator, folly::io::Cursor* c, LengthFieldState* state, size_t* needed) {
  int64_t result = 0;

  if (c->totalLength() < kMinBytesNeeded) {
    *state = LengthFieldState::kMoreBytesNeeded;
    *needed = kMinBytesNeeded - c->totalLength();
    return result;
  }

  std::string field;
  try {
    field = c->readTerminatedString('\r');
  } catch (const std::out_of_range&) {
    // did not find the terminator char
    *state = LengthFieldState::kMoreBytesNeeded;
    *needed = 2;  // '\r\n'
    return result;
  } catch (...) {
    *state = LengthFieldState::kInvalid;
    return result;
  }

  // next character must be '\n'
  if (c->totalLength() == 0) {
    *state = LengthFieldState::kMoreBytesNeeded;
    *needed = 1;  // '\n'
    return result;
  } else if (c->read<char>() != '\n') {
    *state = LengthFieldState::kInvalid;
    return result;
  }

  if (field.size() < 2 || field[0] != typeIndicator) {
    // at least '*' + number
    *state = LengthFieldState::kInvalid;
    return result;
  }

  try {
    result = folly::to<int64_t>(field.substr(1));
    *state = LengthFieldState::kValid;
    return result;
  } catch (std::range_error&) {
    *state = LengthFieldState::kInvalid;
    return result;
  }
}

void RedisDecoder::skipNoise(folly::io::Cursor* c) {
  // skip all the consecutive '\r\n' pairs
  while (c->totalLength() >= 2) {
    const uint8_t* buf = c->data();
    size_t buflen = c->length();

    if (UNLIKELY(buflen < 2)) {
      // not enough bytes left in the current buffer
      // use a tmp cursor to advance two next buffer
      folly::io::Cursor tmp(*c);
      if (tmp.read<char>() == '\r' && tmp.read<char>() == '\n') {
        c->skip(2);
        continue;
      } else {
        return;
      }
    }

    size_t i = 0;
    while (i + 1 < buflen && buf[i] == '\r' && buf[i + 1] == '\n') i += 2;

    c->skip(i);
    // encountered a non-CRLF character
    if (i + 1 < buflen) return;
  }
}

}  // namespace codec
