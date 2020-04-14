//Multi interface Bosch Sensortec BMP280  pressure sensor library
// Copyright (c) 2018 Gregor Christandl <christandlg@yahoo.com>
// home: https://bitbucket.org/christandlg/bmp280mi
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA


#include "BMx280MI.h"

//SPISettings BMx280SPI::spi_settings_ = SPISettings(2000000, MSBFIRST, SPI_MODE1);

BMx280MI::BMx280MI() :
	id_(BMP280_ID),			//assume BMP280 by default
	temp_fine_(0L),
	uh_(0L),
	up_(0L),
	ut_(0L)
{
	comp_params_ = {
		0, //dig_T1
		0, //dig_T2
		0, //dig_T3

		0, //dig_P1
		0, //dig_P2
		0, //dig_P3
		0, //dig_P4
		0, //dig_P5
		0, //dig_P6
		0, //dig_P7
		0, //dig_P8
		0, //dig_P9

		0, //dig_H1
		0, //dig_H2
		0, //dig_H3
		0, //dig_H4
		0, //dig_H5
		0  //dig_H6
	};
}

BMx280MI::~BMx280MI(){
	
}

bool BMx280MI::begin()
{
	//return false if the interface could not be initialized.
	if (!beginInterface())
		return false;

	//update sensor ID.
	id_ = readID();

	//check sensor ID. return false if sensor ID does not match BME280 or BMP280 sensors.
	if ((id_ != BMx280MI::BMP280_ID) && (id_ != BMx280MI::BME280_ID))
		return false;

	//wait until the sensor has finished transferring calibration data
	while (static_cast<bool>(readRegisterValue(BMx280_REG_STATUS, BMx280_MASK_STATUS_IM_UPDATE)))
		delay(100);

	//read compensation parameters
	comp_params_ = readCompensationParameters();

	return true;
}

bool BMx280MI::measure()
{
	//return true if automatic measurements are enabled.
	uint8_t mode = readRegisterValue(BMx280_REG_CTRL_MEAS, BMx280_MASK_MODE);
	if (mode == BMx280_MODE_NORMAL)
		return true;

	//return false if sensor is currently in forced measurement mode.
	else if ((mode == BMx280_MODE_FORCED) || mode == BMx280_MODE_FORCED_ALT)
		return true;

	//start a forced measurement.
	else
		writeRegisterValue(BMx280_REG_CTRL_MEAS, BMx280_MASK_MODE, BMx280_MODE_FORCED);

	return true;
}

bool BMx280MI::hasValue()
{
	bool has_value = !static_cast<bool>(readRegisterValue(BMx280_REG_STATUS, BMx280_MASK_STATUS_MEASURING));

	if (has_value)
	{
		ut_ = readRegisterValueBurst(BMx280_REG_TEMP, BMx280_MASK_TEMP, 3);
		up_ = readRegisterValueBurst(BMx280_REG_PRESS, BMx280_MASK_PRESS, 3);


	}

	return has_value;
}

/*float BMx280MI::getHumidity()
{
	//return NAN if the sensor is not a BME280.
	if (!isBME280())
		return NAN;

	//return NAN if humidity measurements are disabled (humidity value == 0x8000)
	if (uh_ == 0x8000)
		return NAN;

	int32_t v_x1_u32r;

	//code adapted from BME280 data sheet section 4.2.3 and Bosch API
	v_x1_u32r = temp_fine_ - 76800L;
	v_x1_u32r = ((((static_cast<int32_t>(uh_) << 14) - (static_cast<int32_t>(comp_params_.dig_H4_) << 20) - (static_cast<int32_t>(comp_params_.dig_H5_) * v_x1_u32r)) + 16384L) >> 15) *
		(((((((v_x1_u32r * static_cast<int32_t>(comp_params_.dig_H6_)) >> 10) * (((v_x1_u32r * static_cast<int32_t>(comp_params_.dig_H3_)) >> 11) + 32768L)) >> 10) + 2097152L) *
			static_cast<int32_t>(comp_params_.dig_H2_) + 8192L) >> 14);

	v_x1_u32r = v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * static_cast<int32_t>(comp_params_.dig_H1_)) >> 4);

	v_x1_u32r = constrain(v_x1_u32r, 0L, 419430400L);

	return static_cast<float>(v_x1_u32r >> 12) / 1024.0f;
}*/

float BMx280MI::getPressure()
{
	//return NAN if pressure measurements are disabled.
	if (up_ == 0x80000)
		return NAN;

	int32_t var1, var2;
	uint32_t p;

	//code adapted from BME280 data sheet section 8.2 and Bosch API
	var1 = (static_cast<int32_t>(temp_fine_) >> 1) - 64000L;
	var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * (static_cast<int32_t>(comp_params_.dig_P6_));
	var2 = var2 + ((var1*(static_cast<int32_t>(comp_params_.dig_P5_))) << 1);
	var2 = (var2 >> 2) + ((static_cast<int32_t>(comp_params_.dig_P4_)) << 16);
	var1 = (((comp_params_.dig_P3_ * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + (((static_cast<int32_t>(comp_params_.dig_P2_)) * var1) >> 1)) >> 18;
	var1 = ((32768L + var1) * static_cast<int32_t>(comp_params_.dig_P1_)) >> 15;

	if (var1 == 0)
		return NAN; // avoid exception caused by division by zero

	p = ((static_cast<uint32_t>(1048576L - up_)) - (var2 >> 12)) * 3125;

	if (p < 0x80000000)
		p = (p << 1) / static_cast<uint32_t>(var1);
	else
		p = p / static_cast<uint32_t>(var1) * 2;

	var1 = (static_cast<int32_t>(comp_params_.dig_P9_) * static_cast<int32_t>(((p >> 3) * (p >> 3)) >> 13)) >> 12;
	var2 = (static_cast<int32_t>(p >> 2) * static_cast<int32_t>(comp_params_.dig_P8_)) >> 13;
	p = static_cast<uint32_t>(static_cast<int32_t>(p) + ((var1 + var2 + comp_params_.dig_P7_) >> 4));

	return static_cast<float>(p);
}

float BMx280MI::getTemperature()
{
	//return NAN if temperature measurements are disabled.
	if (ut_ == 0x80000)
		return NAN;

	int32_t var1, var2, T;

	//code adapted from BME280 data sheet section 8.2 and Bosch API
	var1 = ((static_cast<int32_t>(ut_) >> 3) - (static_cast<int32_t>(comp_params_.dig_T1_) << 1));
	var1 = (var1 * static_cast<int32_t>(comp_params_.dig_T2_)) >> 11;

	var2 = (static_cast<int32_t>(ut_) >> 4) - static_cast<int32_t>(comp_params_.dig_T1_);
	var2 = ((var2 * var2) >> 12) * static_cast<int32_t>(comp_params_.dig_T3_);
	var2 = var2 >> 14;

	temp_fine_ = var1 + var2;
	T = (temp_fine_ * 5 + 128) >> 8;

	return static_cast<float>(T) / 100.0f;
}

/*float BMx280MI::readHumidity()
{
	if (!isBME280())
		return NAN;

	if (!measure())
		return NAN;

	do
	{
		delay(100);
	} while (!hasValue());

	return getHumidity();
}*/

/*float BMx280MI::readTemperature()
{
	if (!measure())
		return NAN;

	do
	{
		delay(100);
	} while (!hasValue());

	return getTemperature();
}

float BMx280MI::readPressure()
{
	if (!measure())
		return NAN;

	do
	{
		delay(100);
	} while (!hasValue());

	return getPressure();
*/
float BMx280MI::readAltitude()
{
//	float altitude;

  //float pressure = this->getPressure();
  //pressure /= 100; // in Si units for Pascal

 // altitude = 44330 * (1 - pow((this->getPressure()/101325), 0.1903));

  return (44330.0 * (1.0 - pow((this->getPressure()/101325.0), 0.1903)));
}

uint8_t BMx280MI::readID()
{
	return readRegisterValue(BMx280_REG_ID, BMx280_MASK_ID);
}

BMx280MI::BMx280CompParams BMx280MI::readCompensationParameters()
{
	BMx280CompParams comp_params = {
		0, //dig_T1
		0, //dig_T2
		0, //dig_T3

		0, //dig_P1
		0, //dig_P2
		0, //dig_P3
		0, //dig_P4
		0, //dig_P5
		0, //dig_P6
		0, //dig_P7
		0, //dig_P8
		0, //dig_P9

		0, //dig_H1
		0, //dig_H2
		0, //dig_H3
		0, //dig_H4
		0, //dig_H5
		0  //dig_H6
	};

	//read compensation parameters
	comp_params.dig_T1_ = static_cast<uint16_t>(swapByteOrder(readRegisterValueBurst(BMx280_REG_DIG_T1, BMx280_MASK_DIG_T1, 2)));
	comp_params.dig_T2_ = static_cast<int16_t>(swapByteOrder(readRegisterValueBurst(BMx280_REG_DIG_T2, BMx280_MASK_DIG_T2, 2)));
	comp_params.dig_T3_ = static_cast<int16_t>(swapByteOrder(readRegisterValueBurst(BMx280_REG_DIG_T3, BMx280_MASK_DIG_T3, 2)));

	comp_params.dig_P1_ = static_cast<uint16_t>(swapByteOrder(readRegisterValueBurst(BMx280_REG_DIG_P1, BMx280_MASK_DIG_P1, 2)));
	comp_params.dig_P2_ = static_cast<int16_t>(swapByteOrder(readRegisterValueBurst(BMx280_REG_DIG_P2, BMx280_MASK_DIG_P2, 2)));
	comp_params.dig_P3_ = static_cast<int16_t>(swapByteOrder(readRegisterValueBurst(BMx280_REG_DIG_P3, BMx280_MASK_DIG_P3, 2)));
	comp_params.dig_P4_ = static_cast<int16_t>(swapByteOrder(readRegisterValueBurst(BMx280_REG_DIG_P4, BMx280_MASK_DIG_P4, 2)));
	comp_params.dig_P5_ = static_cast<int16_t>(swapByteOrder(readRegisterValueBurst(BMx280_REG_DIG_P5, BMx280_MASK_DIG_P5, 2)));
	comp_params.dig_P6_ = static_cast<int16_t>(swapByteOrder(readRegisterValueBurst(BMx280_REG_DIG_P6, BMx280_MASK_DIG_P6, 2)));
	comp_params.dig_P7_ = static_cast<int16_t>(swapByteOrder(readRegisterValueBurst(BMx280_REG_DIG_P7, BMx280_MASK_DIG_P7, 2)));
	comp_params.dig_P8_ = static_cast<int16_t>(swapByteOrder(readRegisterValueBurst(BMx280_REG_DIG_P8, BMx280_MASK_DIG_P8, 2)));
	comp_params.dig_P9_ = static_cast<int16_t>(swapByteOrder(readRegisterValueBurst(BMx280_REG_DIG_P9, BMx280_MASK_DIG_P9, 2)));

	//reda humidity compensation parameters if the sensor is of the BME280 family
	

	return comp_params;
}

/*bool BMx280MI::isBME280(bool update_id)
{
	if (update_id)
		id_ = readID();

	return (id_ == BMx280MI::BME280_ID);
}*/

void BMx280MI::resetToDefaults()
{
	writeRegisterValue(BMx280_REG_RESET, BMP280_MASK_RESET, BMx280_CMD_RESET);
}

/*uint8_t BMx280MI::readOversamplingHumidity()
{
	return readRegisterValue(BME280_REG_CTRL_HUM, BMx280_MASK_OSRS_H);
}

bool BMx280MI::writeOversamplingHumidity(uint8_t value)
{
	if (!isBME280())
		return false;

	if (value > 0b111)
		return false;

	writeRegisterValue(BME280_REG_CTRL_HUM, BMx280_MASK_OSRS_H, value);

	return true;
}
*/
/*uint8_t BMx280MI::readOversamplingPressure()
{
	return readRegisterValue(BMx280_REG_CTRL_MEAS, BMx280_MASK_OSRS_P);
}*/

bool BMx280MI::writeOversamplingPressure(uint8_t value)
{
	if (value > 0b111)
		return false;

	writeRegisterValue(BMx280_REG_CTRL_MEAS, BMx280_MASK_OSRS_P, value);

	return true;
}

/*uint8_t BMx280MI::readOversamplingTemperature()
{
	return readRegisterValue(BMx280_REG_CTRL_MEAS, BMx280_MASK_OSRS_T);
}*/

bool BMx280MI::writeOversamplingTemperature(uint8_t value)
{
	if (value > 0b111)
		return false;

	writeRegisterValue(BMx280_REG_CTRL_MEAS, BMx280_MASK_OSRS_T, value);

	return true;
}

uint8_t BMx280MI::readFilterSetting()
{
	return readRegisterValue(BMx280_REG_CONFIG, BMx280_MASK_FILTER);
}

bool BMx280MI::writeFilterSetting(uint8_t value)
{
	if (value > 0b111)
		return false;

	writeRegisterValue(BMx280_REG_CONFIG, BMx280_MASK_FILTER, value);

	return true;
}

uint8_t BMx280MI::readPowerMode()
{
	return readRegisterValue(BMx280_REG_CTRL_MEAS, BMx280_MASK_MODE);
}

bool BMx280MI::writePowerMode(uint8_t mode)
{
	if (mode > 0x03)
		return false;

	writeRegisterValue(BMx280_REG_CTRL_MEAS, BMx280_MASK_MODE, mode);

	return true;
}

uint8_t BMx280MI::readStandbyTime()
{
	return readRegisterValue(BMx280_REG_CONFIG, BMx280_MASK_T_SB);
}

bool BMx280MI::writeStandbyTime(uint8_t standby_time)
{
	if (standby_time > 0x07)
		return false;

	writeRegisterValue(BMx280_REG_CONFIG, BMx280_MASK_T_SB, standby_time);

	return true;
}

uint16_t BMx280MI::swapByteOrder(uint16_t data)
{
	//move LSB to MSB and MSB to LSB
	return (data << 8) | (data >> 8);
}

uint8_t BMx280MI::setMaskedBits(uint8_t reg, uint8_t mask, uint8_t value)
{
	//clear mask bits in register
	reg &= (~mask);

	//set masked bits in register according to value
	return ((value << getMaskShift(mask)) & mask) | reg;
}

uint8_t BMx280MI::readRegisterValue(uint8_t reg, uint8_t mask)
{
	return getMaskedBits(readRegister(reg), mask);
}

void BMx280MI::writeRegisterValue(uint8_t reg, uint8_t mask, uint8_t value)
{
	uint8_t reg_val = readRegister(reg);
	writeRegister(reg, setMaskedBits(reg_val, mask, value));
}

uint32_t BMx280MI::readRegisterValueBurst(uint8_t reg, uint32_t mask, uint8_t length)
{
	return getMaskedBits(readRegisterBurst(reg, length), mask);
}

uint32_t BMx280MI::readRegisterBurst(uint8_t reg, uint8_t length)
{
	if (length > 4)
		return 0L;

	uint32_t data = 0L;

	for (uint8_t i = 0; i < length; i++)
	{
		data <<= 8;
		data |= static_cast<uint32_t>(readRegister(reg));
	}

	return data;
}

//-----------------------------------------------------------------------
//BMx280I2C
BMx280I2C::BMx280I2C(uint8_t i2c_address) :
	address_(i2c_address)
{
	//nothing to do here...
}

BMx280I2C::~BMx280I2C()
{
	//nothing to do here...
}

bool BMx280I2C::beginInterface()
{
	return true;
}

uint8_t BMx280I2C::readRegister(uint8_t reg)
{
#if defined(ARDUINO_SAM_DUE)
	//workaround for Arduino Due. The Due seems not to send a repeated start with the code above, so this
	//undocumented feature of Wire::requestFrom() is used. can be used on other Arduinos too (tested on Mega2560)
	//see this thread for more info: https://forum.arduino.cc/index.php?topic=385377.0
	Wire.requestFrom(address_, 1, reg, 1, true);
#else
	Wire.beginTransmission(address_);
	Wire.write(reg);
	Wire.endTransmission(false);
	Wire.requestFrom(address_, static_cast<uint8_t>(1));
#endif

	return Wire.read();
}

uint32_t BMx280I2C::readRegisterBurst(uint8_t reg, uint8_t length)
{
	if (length > 4)
		return 0L;

	uint32_t data = 0L;

#if defined(ARDUINO_SAM_DUE)
	//workaround for Arduino Due. The Due seems not to send a repeated start with the code below, so this
	//undocumented feature of Wire::requestFrom() is used. can be used on other Arduinos too (tested on Mega2560)
	//see this thread for more info: https://forum.arduino.cc/index.php?topic=385377.0
	Wire.requestFrom(address_, length, data, length, true);
#else
	Wire.beginTransmission(address_);
	Wire.write(reg);
	Wire.endTransmission(false);
	Wire.requestFrom(address_, static_cast<uint8_t>(length));

	for (uint8_t i = 0; i < length; i++)
	{
		data <<= 8;
		data |= Wire.read();
	}
#endif

	return data;
}

void BMx280I2C::writeRegister(uint8_t reg, uint8_t value)
{
	Wire.beginTransmission(address_);
	Wire.write(reg);
	Wire.write(value);
	Wire.endTransmission();
}

//-----------------------------------------------------------------------
//BMx280SPI
/*BMx280SPI::BMx280SPI(uint8_t chip_select) :
	cs_(chip_select)
{
	//nothing to do here...
}

BMx280SPI::~BMx280SPI()
{
	//nothing to do here...
}

bool BMx280SPI::beginInterface()
{
	pinMode(cs_, OUTPUT);
	digitalWrite(cs_, HIGH);		//deselect

	return true;
}

uint8_t BMx280SPI::readRegister(uint8_t reg)
{
	uint8_t return_value = 0;

	SPI.beginTransaction(spi_settings_);

	digitalWrite(cs_, LOW);				//select sensor

	SPI.transfer((reg & 0x3F) | 0x40);	//select register and set pin 7 (indicates read)

	return_value = SPI.transfer(0);

	digitalWrite(cs_, HIGH);			//deselect sensor

	return return_value;
}

uint32_t BMx280SPI::readRegisterBurst(uint8_t reg, uint8_t length)
{
	if (length > 4)
		return 0L;

	uint32_t data = 0;

	SPI.beginTransaction(spi_settings_);

	digitalWrite(cs_, LOW);				//select sensor

	SPI.transfer((reg & 0x3F) | 0x40);	//select register and set pin 7 (indicates read)

	for (uint8_t i = 0; i < length; i++)
	{
		data <<= 8;
		data |= SPI.transfer(0);
	}

	digitalWrite(cs_, HIGH);			//deselect sensor

	return data;
}

void BMx280SPI::writeRegister(uint8_t reg, uint8_t value)
{
	SPI.beginTransaction(spi_settings_);

	digitalWrite(cs_, LOW);				//select sensor

	SPI.transfer((reg & 0x3F));			//select regsiter
	SPI.transfer(value);

	digitalWrite(cs_, HIGH);			//deselect sensor
}*/