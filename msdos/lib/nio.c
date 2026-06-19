#include "nio.h"
#include "serial.h"

#include <string.h>

#define NIO_TIMEOUT_MS 15000U
#define NIO_MAX_RX 1536U

enum {
  SLIP_END = 0xC0,
  SLIP_ESCAPE = 0xDB,
  SLIP_ESC_END = 0xDC,
  SLIP_ESC_ESC = 0xDD
};

enum {
  FUJI_FIELD_NONE = 0,
  FUJI_FIELD_A1 = 1
};

typedef struct {
  uint8_t device;
  uint8_t command;
  uint16_t length;
  uint8_t checksum;
  uint8_t fields;
} nio_header_t;

static uint8_t rx_frame[NIO_MAX_RX];
static uint8_t rx_payload[NIO_MAX_RX];

static void put_u16le(uint8_t *p, uint16_t value)
{
  p[0] = (uint8_t) (value & 0xFF);
  p[1] = (uint8_t) ((value >> 8) & 0xFF);
}

static void put_u32le(uint8_t *p, uint32_t value)
{
  p[0] = (uint8_t) (value & 0xFF);
  p[1] = (uint8_t) ((value >> 8) & 0xFF);
  p[2] = (uint8_t) ((value >> 16) & 0xFF);
  p[3] = (uint8_t) ((value >> 24) & 0xFF);
}

static uint16_t get_u16le(const uint8_t *p)
{
  return (uint16_t) p[0] | ((uint16_t) p[1] << 8);
}

static uint32_t get_u32le(const uint8_t *p)
{
  return (uint32_t) p[0]
      | ((uint32_t) p[1] << 8)
      | ((uint32_t) p[2] << 16)
      | ((uint32_t) p[3] << 24);
}

static uint16_t nio_calc_checksum(const void *ptr, uint16_t len, uint16_t seed)
{
  uint16_t idx;
  uint16_t chk = seed;
  const uint8_t *buf = (const uint8_t *) ptr;

  for (idx = 0; idx < len; idx++)
    chk = ((chk + buf[idx]) >> 8) + ((chk + buf[idx]) & 0xFF);
  return chk;
}

static void slip_put_byte(uint8_t value)
{
  if (value == SLIP_END) {
    serial_put_byte(SLIP_ESCAPE);
    serial_put_byte(SLIP_ESC_END);
  } else if (value == SLIP_ESCAPE) {
    serial_put_byte(SLIP_ESCAPE);
    serial_put_byte(SLIP_ESC_ESC);
  } else {
    serial_put_byte(value);
  }
}

static void slip_put_buf(const void *ptr, uint16_t len)
{
  uint16_t idx;
  const uint8_t *buf = (const uint8_t *) ptr;

  for (idx = 0; idx < len; idx++)
    slip_put_byte(buf[idx]);
}

static uint16_t slip_get_frame(uint8_t *buf, uint16_t cap, unsigned timeout_ms)
{
  uint16_t len = 0;
  int started = 0;
  int escaped = 0;
  uint8_t value;

  while (serial_get_byte(&value, timeout_ms)) {
    if (value == SLIP_END) {
      if (started && len)
        return len;
      started = 1;
      len = 0;
      escaped = 0;
      continue;
    }

    if (!started)
      continue;

    if (escaped) {
      if (value == SLIP_ESC_END)
        value = SLIP_END;
      else if (value == SLIP_ESC_ESC)
        value = SLIP_ESCAPE;
      else
        return 0;
      escaped = 0;
    } else if (value == SLIP_ESCAPE) {
      escaped = 1;
      continue;
    }

    if (len >= cap)
      return 0;
    buf[len++] = value;
  }

  return 0;
}

const char *nio_status_name(uint8_t status)
{
  switch (status) {
  case NIO_STATUS_OK:
    return "OK";
  case NIO_STATUS_DEVICE_NOT_FOUND:
    return "DEVICE_NOT_FOUND";
  case NIO_STATUS_INVALID_REQUEST:
    return "INVALID_REQUEST";
  case NIO_STATUS_DEVICE_BUSY:
    return "DEVICE_BUSY";
  case NIO_STATUS_NOT_READY:
    return "NOT_READY";
  case NIO_STATUS_IO_ERROR:
    return "IO_ERROR";
  case NIO_STATUS_TIMEOUT:
    return "TIMEOUT";
  case NIO_STATUS_INTERNAL_ERROR:
    return "INTERNAL_ERROR";
  case NIO_STATUS_UNSUPPORTED:
    return "UNSUPPORTED";
  default:
    return "UNKNOWN";
  }
}

int nio_call(uint8_t device, uint8_t command,
             const void *payload, uint16_t payload_length,
             void *reply, uint16_t reply_capacity,
             nio_response_t *response)
{
  nio_header_t tx;
  nio_header_t *rx;
  uint16_t checksum;
  uint16_t rx_len;
  uint16_t rx_payload_len;
  uint8_t status;

  if (response) {
    response->status = NIO_STATUS_INTERNAL_ERROR;
    response->payload_length = 0;
  }

  tx.device = device;
  tx.command = command;
  tx.length = sizeof(tx) + payload_length;
  tx.checksum = 0;
  tx.fields = FUJI_FIELD_NONE;

  checksum = nio_calc_checksum(&tx, sizeof(tx), 0);
  if (payload && payload_length)
    checksum = nio_calc_checksum(payload, payload_length, checksum);
  tx.checksum = (uint8_t) checksum;

  serial_drain_rx();
  serial_put_byte(SLIP_END);
  slip_put_buf(&tx, sizeof(tx));
  if (payload && payload_length)
    slip_put_buf(payload, payload_length);
  serial_put_byte(SLIP_END);

  rx_len = slip_get_frame(rx_frame, sizeof(rx_frame), NIO_TIMEOUT_MS);
  if (rx_len < sizeof(nio_header_t))
    return 0;

  rx = (nio_header_t *) rx_frame;
  if (rx_len != rx->length)
    return 0;

  rx_payload_len = rx_len - sizeof(nio_header_t);
  checksum = rx->checksum;
  rx->checksum = 0;
  if ((uint8_t) nio_calc_checksum(rx_frame + sizeof(nio_header_t), rx_payload_len,
        nio_calc_checksum(rx, sizeof(nio_header_t), 0)) != checksum)
    return 0;

  if (rx->device != device || rx->command != command)
    return 0;
  if ((rx->fields & 0x80) || (rx->fields & 0x07) != FUJI_FIELD_A1)
    return 0;
  if (rx_payload_len < 1)
    return 0;

  status = rx_frame[sizeof(nio_header_t)];
  rx_payload_len--;
  if (rx_payload_len > reply_capacity)
    return 0;
  if (reply && rx_payload_len)
    memcpy(reply, rx_frame + sizeof(nio_header_t) + 1, rx_payload_len);

  if (response) {
    response->status = status;
    response->payload_length = rx_payload_len;
  }

  return 1;
}

int nio_disk_info(uint8_t slot, nio_disk_info_t *info, nio_response_t *response)
{
  uint8_t req[2];
  uint8_t resp[16];
  nio_response_t local_response;

  req[0] = NIO_DISK_VERSION;
  req[1] = slot;

  if (!response)
    response = &local_response;
  if (!nio_call(NIO_DEVICEID_DISK, NIO_DISK_CMD_INFO,
                req, sizeof(req), resp, sizeof(resp), response))
    return 0;
  if (response->status != NIO_STATUS_OK || response->payload_length < 12 || resp[0] != NIO_DISK_VERSION)
    return 0;

  info->flags = resp[1];
  info->slot = resp[4];
  info->image_type = resp[5];
  info->sector_size = get_u16le(&resp[6]);
  info->sector_count = get_u32le(&resp[8]);
  info->last_error = (response->payload_length > 12) ? resp[12] : 0;
  return 1;
}

int nio_disk_read_sector(uint8_t slot, uint32_t lba,
                         void *buffer, uint16_t buffer_length,
                         uint16_t *bytes_read, nio_response_t *response)
{
  uint8_t req[8];
  nio_response_t local_response;
  uint16_t data_len;

  req[0] = NIO_DISK_VERSION;
  req[1] = slot;
  put_u32le(&req[2], lba);
  put_u16le(&req[6], buffer_length);

  if (!response)
    response = &local_response;
  if (!nio_call(NIO_DEVICEID_DISK, NIO_DISK_CMD_READ_SECTOR,
                req, sizeof(req), rx_payload, sizeof(rx_payload), response))
    return 0;
  if (response->status != NIO_STATUS_OK || response->payload_length < 11 || rx_payload[0] != NIO_DISK_VERSION)
    return 0;

  data_len = get_u16le(&rx_payload[9]);
  if (data_len > buffer_length || response->payload_length < (uint16_t) (11 + data_len))
    return 0;

  if (buffer && data_len)
    memcpy(buffer, rx_payload + 11, data_len);
  if (bytes_read)
    *bytes_read = data_len;
  return 1;
}
