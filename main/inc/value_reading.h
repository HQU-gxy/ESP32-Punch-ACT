//
// Created by Kurosu Chan on 2023/12/5.
//

#ifndef PUNCHER_VALUE_READING_H
#define PUNCHER_VALUE_READING_H

#include <etl/expected.h>
#include <cbor.h>
#include <variant>

namespace protocol {
// https://gist.github.com/soburi/4e1cc77df363e52ff0f366aeb23dac39
// https://www.rfc-editor.org/rfc/rfc8949.html#name-cbor-data-models
// https://www.rfc-editor.org/rfc/rfc8949.html#name-cbor-tags-registry
// https://www.rfc-editor.org/rfc/rfc8949.html#name-tagging-of-items

enum class Command {
  ONCE        = 0x12,
  SUCCESSIVE  = 0x13,
  STOP        = 0x14,
  TARE        = 0x20,
  BTN_DISABLE = 0x30,
  BTN_ENABLE  = 0x31,
  UNKNOWN,
};

struct change_duration_t {
  static constexpr uint8_t MAGIC = 0x40;
  int duration                   = 0;
};

using request_t = std::variant<Command, change_duration_t>;

/**
 * @brief encode a load cell reading into a CBOR byte array
 * @tparam It an iterator type that points to a pair of float and uint16_t
 */
template <typename It>
  requires std::input_iterator<It>
etl::expected<size_t, CborError>
encode_load_cell_reading(const It begin,
                         const It end,
                         uint8_t *buffer,
                         size_t size) {
  using ue_t = etl::unexpected<CborError>;
  CborError err;
  CborEncoder encoder;
  cbor_encoder_init(&encoder, buffer, size, 0);
  const auto len = std::distance(begin, end) * 2;
  CborEncoder container;
  err = cbor_encoder_create_array(&encoder, &container, len);
  if (err != CborNoError) {
    return ue_t{err};
  }

  // using pair_t = std::tuple<float, uint16_t>;
  for (auto it = begin; it != end; ++it) {
    const auto &pair = *it;
    err              = cbor_encode_float_as_half_float(&container, std::get<0>(pair));
    if (err != CborNoError) {
      return ue_t{err};
    }
    err = cbor_encode_uint(&container, std::get<1>(pair));
    if (err != CborNoError) {
      return ue_t{err};
    }
  }
  err = cbor_encoder_close_container_checked(&encoder, &container);
  if (err != CborNoError) {
    return ue_t{err};
  }
  size_t sz = cbor_encoder_get_buffer_size(&encoder, buffer);
  return sz;
};

etl::expected<request_t, CborError>
decode_command(const uint8_t *buffer, size_t size);
}

#endif // PUNCHER_VALUE_READING_H
