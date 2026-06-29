#include "nio.h"

#include "fn_raw.h"

#include <string.h>

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

static uint8_t map_fn_error(uint8_t err)
{
  switch (err) {
  case 0: return NIO_STATUS_OK;
  case 1: return NIO_STATUS_DEVICE_NOT_FOUND;
  case 2: return NIO_STATUS_INVALID_REQUEST;
  case 3: return NIO_STATUS_DEVICE_BUSY;
  case 4: return NIO_STATUS_NOT_READY;
  case 5: return NIO_STATUS_IO_ERROR;
  case 6: return NIO_STATUS_TIMEOUT;
  case 8: return NIO_STATUS_UNSUPPORTED;
  default: return NIO_STATUS_INTERNAL_ERROR;
  }
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
  fn_raw_response_t raw;
  uint8_t result;

  if (response) {
    response->status = NIO_STATUS_INTERNAL_ERROR;
    response->payload_length = 0;
  }

  result = fn_raw_call(device, command, payload, payload_length,
                       reply, reply_capacity, &raw);
  if (result != 0) {
    if (response)
      response->status = map_fn_error(result);
    return 0;
  }

  if (response) {
    response->status = raw.status;
    response->payload_length = raw.payload_length;
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
  uint8_t resp[1024];
  nio_response_t local_response;
  uint16_t data_len;

  req[0] = NIO_DISK_VERSION;
  req[1] = slot;
  put_u32le(&req[2], lba);
  put_u16le(&req[6], buffer_length);

  if (!response)
    response = &local_response;
  if (!nio_call(NIO_DEVICEID_DISK, NIO_DISK_CMD_READ_SECTOR,
                req, sizeof(req), resp, sizeof(resp), response))
    return 0;
  if (response->status != NIO_STATUS_OK || response->payload_length < 11 || resp[0] != NIO_DISK_VERSION)
    return 0;

  data_len = get_u16le(&resp[9]);
  if (data_len > buffer_length || response->payload_length < (uint16_t) (11 + data_len))
    return 0;

  if (buffer && data_len)
    memcpy(buffer, resp + 11, data_len);
  if (bytes_read)
    *bytes_read = data_len;
  return 1;
}
