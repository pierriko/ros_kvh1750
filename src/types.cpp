/**
 * Implementation of KVH 1750 types.
 * \author Jason Ziglar <jpz@vt.edu>
 * \date 02/16/2015
 */
#include "kvh1750/types.h"

#include <byteswap.h>
#include <boost/crc.hpp>

namespace
{
  const double CF_Offset = 32.0;
  const double FC_Scale = 5.0 / 9.0;
  const double CF_Scale = 9.0 / 5.0;
}

namespace kvh
{

/**
 * Default Constructor, initializes to invalid memory
 */
Message::Message() :
  _ang_vel{0.0, 0.0, 0.0},
  _lin_accel{0.0, 0.0, 0.0},
  _secs(0),
  _nsecs(0),
  _temp(0),
  _status(0),
  _seq(0),
  _is_da(false),
  _is_c(false)
{
}

/**
 * Constructor which converts a raw message to a valid message
 */
Message::Message(const RawMessage& raw, uint32_t secs, uint32_t nsecs, bool is_c,
  bool is_da) :
  _ang_vel(),
  _lin_accel(),
  _secs(),
  _nsecs(),
  _temp(),
  _status(),
  _seq(),
  _is_da(),
  _is_c()
{
  from_raw(raw, secs, nsecs, is_c, is_da);
}

Message::~Message()
{
}

bool Message::from_raw(const RawMessage& raw, uint32_t secs, uint32_t nsecs,
  bool is_c, bool is_da)
{
  bool valid_msg = valid_checksum(raw);

  if(valid_msg)
  {
    const size_t Num_Values_Per_Array = 3;

    for(size_t ii = 0; ii < Num_Values_Per_Array; ++ii)
    {

      union swap_float
      {
        int32_t raw;
        float val;
      } ang, lin;

      ang.raw = bswap_32(raw.rots[ii]);
      lin.raw = bswap_32(raw.accels[ii]);
      _ang_vel[ii] = ang.val;
      _lin_accel[ii] = Gravity * lin.val;
    }

    _status = raw.status;
    _seq = raw.seq;
    _temp = bswap_16(raw.temp);
    _secs = secs;
    _nsecs = nsecs;
    _is_c = is_c;
    _is_da = is_da;
  }
  else
  {
    *this = Message(); //zero all data
  }

  return valid_msg;
}

float Message::gyro_x() const
{
  return _ang_vel[X];
}

float Message::gyro_y() const
{
  return _ang_vel[Y];
}

float Message::gyro_z() const
{
  return _ang_vel[Z];
}

float Message::accel_x() const
{
  return _lin_accel[X];
}

float Message::accel_y() const
{
  return _lin_accel[Y];
}

float Message::accel_z() const
{
  return _lin_accel[Z];
}

int16_t Message::temp() const
{
  return _temp;
}

int16_t Message::temp(bool& is_c) const
{
  is_c = _is_c;
  return _temp;
}

void Message::time(uint32_t& secs, uint32_t& nsecs) const
{
  secs = _secs;
  nsecs = _nsecs;
}

uint8_t Message::sequence_number() const
{
  return _seq;
}

/**
 * Tests if this message contains valid data.
 */
bool Message::valid() const
{
  return _status != 0;
}

bool Message::valid_gyro_x() const
{
  return _status & GYRO_X;
}

bool Message::valid_gyro_y() const
{
  return _status & GYRO_Y;
}

bool Message::valid_gyro_z() const
{
  return _status & GYRO_Z;
}

bool Message::valid_accel_x() const
{
  return _status & ACCEL_X;
}

bool Message::valid_accel_y() const
{
  return _status & ACCEL_Y;
}

bool Message::valid_accel_z() const
{
  return _status & ACCEL_Z;
}

bool Message::is_celsius() const
{
  return _is_c;
}

bool Message::is_delta_angle() const
{
  return _is_da;
}

void Message::to_celsius()
{
  if(_is_c)
  {
    return;
  }

  _temp = static_cast<int16_t>((static_cast<double>(_temp) - CF_Offset) * FC_Scale);
  _is_c = true;
}

void Message::to_farenheit()
{
  if(!_is_c)
  {
    return;
  }

  _temp = static_cast<int16_t>((static_cast<double>(_temp) * CF_Scale) + CF_Offset);
  _is_c = false;
}

/**
 * Function to test the CRC of a RawMessage using the parameters
 * defined by KVH.
 * \param[in] Message to test
 * \param[out] Flag indicating if CRC matches.
 */
bool valid_checksum(const kvh::RawMessage& msg)
{
  const size_t CRCSize = sizeof(kvh::RawMessage) - sizeof(msg.crc);
  const char* ptr = reinterpret_cast<const char*>(&msg);
  uint32_t check = compute_checksum(ptr, CRCSize);

  return (check == bswap_32(msg.crc));
}

/**
 * Computes checksum of an arbitrary buffer of a given length,
 * using the KVH CRC parameters
 * \param[in] buff Pointer to buffer
 * \param[in] len Length of buffer in bytes
 * \param[out] CRC checksum value
 */
uint32_t compute_checksum(const char* buff, size_t len)
{
  return boost::crc<crc::Width, crc::Poly, crc::XOr_In, crc::XOr_Out,
    crc::Reflect_In, crc::Reflect_Out>(buff, len);
}

}