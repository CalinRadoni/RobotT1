#ifndef HIHSensor_H
#define HIHSensor_H

#include <Arduino.h>

class HIHSensor
{
public:
    HIHSensor(void);
    virtual ~HIHSensor();

	enum class Status : uint8_t {
		DATA_OK,
		DATA_Stale,
		DATA_Err
	};

    bool IsPresent(void);

	/**
	 * \brief Begin measurement cycle
	 *
	 * \details Call this function to start a measurement cycle.
	 * The measurement cycle duration is typically 36.65 ms for temperature and humidity readings.
	 */
    bool ReadInit(void);

	/**
	 * \brief Read measured data
     *
	 * \return Status::DATA_Err for errors
	 *         Status::DATA_OK if data is OK
	 *         Status::DATA_Stale if data is old
	 */
	HIHSensor::Status ReadData(void);

    /**
     * \brief Check if a read is in progress
     *
     * \return true if a read is in progress
     */
    bool ReadInProgress(void);

	/**
	 * \brief Converts rawH to 10*humidity[%RH]
	 *
	 * rawH interval is [0 ... (2^14 - 2)] for [0 ... 100] %RH
	 * output interval is [0 ... 1000]
	 */
	uint16_t GetHumidity(void);

	/** \brief Converts rawT to 10*temperature[degK]
	 *
	 * rawT interval is [0 ... (2^14 - 2)] for [-40 ... +125] degC
	 * output interval is [2331 ... 3981]. To convert in 10*degC substract 2731
	 */
	uint16_t GetTemperature(void);

protected:
    uint16_t rawH;
    uint16_t rawT;

    bool readInProgress;
};

#endif
