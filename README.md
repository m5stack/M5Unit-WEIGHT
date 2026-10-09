# M5Unit - WEIGHT

## Overview

Library for WEIGHT using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### SKU:U180

The Weight I2C Unit is a weight acquisition transducer unit that employs the "STM32+HX711 chip" solution, achieving precision weight measurement with 24-bit accuracy through I2C communication. It supports the parallel connection of multiple devices on the same I2C bus, providing users with greater flexibility in terms of a wider range and more data collection points. It is suitable for various applications such as industrial production, healthcare, logistics, laboratory research, and food processing.

### SKU:U177

Mini Scales Unit is an integrated mini weighing unit that combines a 5kg weighing sensor, an ADC acquisition chip (HX711), and a protocol conversion MCU to directly output weighing results. The product is equipped with an STM32F030F4P6 microcontroller, uses the I2C communication protocol for external communication, and supports multiple modules working together. It also features an SK6812 RGB LED and a function button, making it easy to perform zero calibration and tare.

## Related Link

- [Unit WeightI2C & Datasheet](https://docs.m5stack.com/en/unit/Unit-Weight%20I2C)
- [Unit MiniScales & Datasheet](https://docs.m5stack.com/en/unit/Unit-Mini%20Scales)

### Required Libraries:
- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

## License

- [M5Unit-WEIGHT - MIT](LICENSE)

## Examples
See also [examples/UnitUnified](examples/UnitUnified)

- UnitWeightI2C/PlotToSerial: Outputs the weight of each periodic measurement for a serial plotter
- UnitMiniScales/PlotToSerial: The same, and changes the LED color with the weight
- ViaPaHub: Uses UnitWeightI2C (ch:0) and UnitMiniScales (ch:1) through one PaHub (requires [M5Unit-HUB](https://github.com/m5stack/M5Unit-HUB))

### For ESP-IDF settings

> **NOTE:** The ESP-IDF native build (`idf.py`) targets ESP-IDF **5.1 or later** (5.x and 6.x).

Each example covers a single unit (or a fixed set of units for ViaPaHub), so the examples need no Kconfig choice. Build them directly:

```sh
cd examples/UnitUnified/UnitWeightI2C/PlotToSerial
idf.py set-target esp32s3               # or esp32 / esp32c5 / esp32c6 / esp32h2 / esp32p4 / ...
idf.py build flash monitor
```

## Support via [PaHub](https://docs.m5stack.com/en/unit/Unit-PaHub%20v2.1)

|Unit|Support|Note|
|---|---|---|
|UnitWeightI2C|OK||
|UnitMiniScales|OK||

Both units use the same I2C address (0x26). Connect them to different PaHub channels to use them at the same time.

See also the [ViaPaHub](examples/UnitUnified/ViaPaHub) example (UnitWeightI2C / UnitMiniScales on one PaHub) and [M5Unit-HUB](https://github.com/m5stack/M5Unit-HUB)



### Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-WEIGHT/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

#### Required
- [Doxygen](https://www.doxygen.nl/)
- [Git](https://git-scm.com/) (Output commit hash to html)

