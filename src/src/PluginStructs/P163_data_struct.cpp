#include "../PluginStructs/P163_data_struct.h"

#ifdef USES_P163

/**************************************************************************
* Constructor
**************************************************************************/
P163_data_struct::P163_data_struct(struct EventStruct *event) {
  _lowPowerMode = P163_GET_LOW_POWER;
  _ledState     = P163_GET_LED_STATE;
  _threshold    = P163_CFG_THRESHOLD;
  _changeOnly   = _threshold < 0;
}

P163_data_struct::~P163_data_struct() {
  delete sensor;
}

/*****************************************************
* init
*****************************************************/
bool P163_data_struct::init(struct EventStruct *event) {
  sensor = new (std::nothrow) CG_RadSens(RS_DEFAULT_I2C_ADDRESS);

  if ((nullptr != sensor) && sensor->init()) {
    initialized = true;

    sensor->setLPmode(_lowPowerMode);
    sensor->setLedState(_ledState);

    if (loglevelActiveFor(LOG_LEVEL_INFO)) {
      addLog(LOG_LEVEL_INFO, strformat(F("RadSens: Initialized, ChipID: 0x%02x, Firmware: %d"),
                                       sensor->getChipId(), sensor->getFirmwareVersion()));
    }
  } else {
    addLog(LOG_LEVEL_ERROR, F("RadSens: Initialization failed!"));
  }
  return isInitialized();
}

/*****************************************************
* plugin_read
*****************************************************/
bool P163_data_struct::plugin_read(struct EventStruct *event) {
  if (isInitialized()) {
    if (setOutputValues(event)) {
      return true;
    }
  }
  return false;
}

bool P163_data_struct::setOutputValues(struct EventStruct *event) {
  bool result = false;

  const uint32_t count    = sensor->getNumberOfPulses();
  const float    iDynamic = sensor->getRadIntensyDynamic();
  const float    iStatic  = sensor->getRadIntensyStatic();
  const int32_t  delta    = abs(count - UserVar.getFloat(event->TaskIndex, 0));

  result = !_changeOnly || (delta >= _threshold);

  UserVar.setFloat(event->TaskIndex, 0, count);

  UserVar.setFloat(event->TaskIndex, 1, iDynamic);
  UserVar.setFloat(event->TaskIndex, 2, iStatic);

  return result;
}

/*****************************************************
* plugin_write
*****************************************************/
const char P163_subcommands[] PROGMEM = "calibration|";

enum class P163_subcmd_e : int8_t {
  invalid     = -1,
  calibration = 0,
};

bool P163_data_struct::plugin_write(struct EventStruct *event,
                                    String            & string) {
  bool success = false;

  const String command = parseString(string, 1);

  if (isInitialized() && equals(command, F("radsens"))) {
    const String subcommand   = parseString(string, 2);
    const int    subcommand_i = GetCommandCode(subcommand.c_str(), P163_subcommands);

    if (subcommand_i < 0) { return false; } // Fail fast

    const P163_subcmd_e subcmd = static_cast<P163_subcmd_e>(subcommand_i);

    switch (subcmd) {
      case P163_subcmd_e::invalid:
        break;
      case P163_subcmd_e::calibration:

        if (event->Par2 >= 0) {
          sensor->setSensitivity(event->Par2);
          success = true;
        }
        break;
    }
  }
  return success;
}

#endif // ifdef USES_P163
