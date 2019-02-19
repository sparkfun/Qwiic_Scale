/*

*/
const byte _deviceAddress = 0x2A; //Default address of the NAU7802

#include <Wire.h>

#include "registers.h"

void setup()
{
  Serial.begin(9600);
  Serial.println("Qwiic Scale Example");

  Wire.begin();

  if (isConnected == false)
  {
    Serial.println("Scale not detected. Please check wiring. Freezing...");
    while (1);
  }
  Serial.println("Scale detected!");
}

void loop()
{

}

//Inits the sensor
//Returns true upon completion
bool begin()
{
  //Check if the device ack's over I2C
  if(isConnected() == false) return(false);

  reset(); //Reset all registers

  powerUp(); //Power on analog and digital sections of the scale

  //Wait for Power Up bit to be set
  uint8_t counter = 0;
  while(1)
  {
    if(getBit(NAU7802_PU_CTRL_PUR, NAU7802_PU_CTRL) == true) break; //Good to go
    delay(1);
    if(counter++ > 100) return(false); //Error
  }
  
  setLDO(NAU7802_LDO_3V3); //Set LDO to 3.3V

  setGain(NAU7802_GAIN_16); //Set gain to 16

  setConversionRate(NAU7802_SPS_10); //Set samples per second to 10

  //calibrate();
  
  return(true);  
}

//Returns true if device is present
//Tests for device ack to I2C address
bool isConnected()
{
  return (true); //Return true if Power Up Ready is set
}

//Calibrate system. Returns true if CAL_ERR bit is 0 (no error)
bool calibrate(bool waitForCompletion)
{
  setBit(NAU7802_CTRL2_CALS, NAU7802_CTRL2); //Begin calibration

  if(waitForCompletion == true)
  {
    uint16_t counter = 0;

    while(1)
    {
      if(getBit(NAU7802_CTRL2_CALS, NAU7802_CTRL2) == false) break; //Goes to 0 once cal is complete
      delay(1);
      if(counter++ > 1000) return(false);
    }
  }

  if(getBit(NAU7802_CTRL2_CAL_ERROR, NAU7802_CTRL2) == false) return(true); //No error! Cal is good.
  return(false); //Cal error
  
}

//Set the readings per second
//10, 20, 40, 80, and 320 samples per second is available
bool setConversionRate(uint8_t rate)
{
  if(rate > 0b111) rate = 0b111; //Error check
  
  uint8_t value = getRegister(NAU7802_CTRL2);
  value &= 0b10001111; //Clear CRS bits
  value |= rate << 4; //Mask in new CRS bits

  return(setRegister(NAU7802_CTRL2, value));
}

//Select between 1 and 2
bool setChannel(uint8_t channelNumber)
{
  if(channelNumber == 0)
    return(clearBit(NAU7802_CTRL2_CHS, NAU7802_CTRL2)); //Channel 1 (default)
  else
    return(setBit(NAU7802_CTRL2_CHS, NAU7802_CTRL2)); //Channel 2
}

//Power up digital and analog sections of scale
bool powerUp()
{
  setBit(NAU7802_PU_CTRL_PUD, NAU7802_PU_CTRL);
  return(setBit(NAU7802_PU_CTRL_PUA, NAU7802_PU_CTRL));
}

//Puts scale into low-power mode
bool powerDown()
{
  clearBit(NAU7802_PU_CTRL_PUD, NAU7802_PU_CTRL);
  return(clearBit(NAU7802_PU_CTRL_PUA, NAU7802_PU_CTRL));
}

//Resets the NAU7802
bool reset()
{
  setBit(NAU7802_PU_CTRL_RR, NAU7802_PU_CTRL); //Set RR
  delay(1);
  //Do we have to clear RR or does it do it automatically?
  return(clearBit(NAU7802_PU_CTRL_RR, NAU7802_PU_CTRL)); //Clear RR
}

//Set the onboard Low-Drop-Out voltage regulator to a given value
//2.4, 2.7, 3.0, 3.3, 3.6, 3.9, 4.2, 4.5V are avaialable
bool setLDO(uint8_t ldoValue)
{
  if(ldoValue > 0b111) ldoValue = 0b111; //Error check
  
  uint8_t value = getRegister(NAU7802_CTRL1);
  value &= 0b11000111; //Clear LDO bits
  value |= ldoValue << 3; //Mask in new LDO bits

  return(setRegister(NAU7802_CTRL1, value));
}

//Set the gain
//x1, 2, 4, 8, 16, 32, 64, 128 are avaialable
bool setGain(uint8_t gainValue)
{
  if(gainValue > 0b111) gainValue = 0b111; //Error check
  
  uint8_t value = getRegister(NAU7802_CTRL1);
  value &= 0b11111000; //Clear gain bits
  value |= gainValue; //Mask in new bits

  return(setRegister(NAU7802_CTRL1, value));
}

//Returns 24-bit reading
//Assumes CR Cycle Ready byte (ADC conversion complete) has been checked to be 1
uint32_t getScaleReading()
{
  Wire.beginTransmission(_deviceAddress);
  Wire.write(NAU7802_ADCO_B2);
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK

  Wire.requestFrom((uint8_t)_deviceAddress, (uint8_t)3);

  if (Wire.available())
  {
    uint32_t value = (uint32_t)Wire.read() << 16; //MSB
    value |= (uint32_t)Wire.read() << 8; //MidSB
    value |= (uint32_t)Wire.read(); //LSB
    return (value);
  }

  return (0); //Error
}

//Mask & set a given bit within a register
bool setBit(uint8_t bitNumber, uint8_t registerAddress)
{
  uint8_t value = getRegister(registerAddress);
  value |= (1 << bitNumber); //Set this bit
  return (setRegister(registerAddress, value));
}

//Mask & clear a given bit within a register
bool clearBit(uint8_t bitNumber, uint8_t registerAddress)
{
  uint8_t value = getRegister(registerAddress);
  value &= ~(1 << bitNumber); //Set this bit
  return (setRegister(registerAddress, value));
}

//Return a given bit within a register
bool getBit(uint8_t bitNumber, uint8_t registerAddress)
{
  uint8_t value = getRegister(registerAddress);
  value &= (1 << bitNumber); //Clear all but this bit
  return (value);
}

//Get contents of a register
uint8_t getRegister(uint8_t registerAddress)
{
  Wire.beginTransmission(_deviceAddress);
  Wire.write(registerAddress);
  if (Wire.endTransmission() != 0)
    return (-1); //Sensor did not ACK

  Wire.requestFrom((uint8_t)_deviceAddress, (uint8_t)1);

  if (Wire.available())
    return (Wire.read());

  return (-1); //Error
}

//Send a given value to be written to given address
//Return true if successful
bool setRegister(uint8_t registerAddress, uint8_t value)
{
  Wire.beginTransmission(_deviceAddress);
  Wire.write(registerAddress);
  Wire.write(value);
  if (Wire.endTransmission() != 0)
    return (false); //Sensor did not ACK
  return (true);
}
