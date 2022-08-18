#include "HIHSensor.h"

#include "Wire.h"

const uint16_t sensorAddr = 0x27;

HIHSensor::HIHSensor(void)
{
    rawH = 0;
    rawT = 0;

    readInProgress = false;
}

HIHSensor::~HIHSensor()
{
    //
}

bool HIHSensor::IsPresent(void)
{
    Wire.beginTransmission(sensorAddr);
    uint8_t err = Wire.endTransmission();
    if (0 == err) {
        return true;
    }

    log_e("HIH Sensor detection error %d at address 0x%02X", err, sensorAddr);
    return false;
}

bool HIHSensor::ReadInit(void)
{
    Wire.beginTransmission(sensorAddr);
    if (Wire.endTransmission() != 0) {
        return false;
    }

    readInProgress = true;
    return true;
}

HIHSensor::Status HIHSensor::ReadData(void)
{
    const uint8_t dataLEN = 4;

    uint8_t bytesReceived = Wire.requestFrom(sensorAddr, dataLEN);
    if (dataLEN != bytesReceived) {
        log_e("Received %d bytes instead of %d", bytesReceived, dataLEN);
        readInProgress = false;
        return HIHSensor::Status::DATA_Err;
    }

    uint8_t buffer[dataLEN];
    Wire.readBytes(buffer, dataLEN);

    uint8_t stale = buffer[0] & 0xC0;

    rawH  = buffer[0] & 0x3F;
    rawH  = rawH << 8;
    rawH += buffer[1];

    rawT  = buffer[2];
    rawT  = rawT << 8;
    rawT += buffer[3];
    rawT  = rawT >> 2;

    if(stale == 0x00) {
        readInProgress = false;
        return HIHSensor::Status::DATA_OK;
    }

    if(stale == 0x40) {
        return HIHSensor::Status::DATA_Stale;
    }

    readInProgress = false;
    return HIHSensor::Status::DATA_Err;
}

uint16_t HIHSensor::GetHumidity(void)
{
	uint32_t val;

	val = rawH;
	val *= 5000;
	val /= 8191;
	val += 5;
	val /= 10;
	return (uint16_t)val;
}

uint16_t HIHSensor::GetTemperature(void)
{
	uint32_t val;

	val = rawT;
	val *= 8250;
	val /= 8191;
	val += 23310;
	val += 5;
	val /= 10;
	return (uint16_t)val;
}

bool HIHSensor::ReadInProgress(void)
{
    return readInProgress;
}
