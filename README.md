# yambar-sensors

yambar-sensors is a user module for the yambar modular status panel that
provides access to sensor data from libsensors. It allows users to display
various hardware sensor readings (such as temperature, fan speed, and voltage)
in their yambar status panel.

## Prerequisites

- yambar (https://codeberg.org/dnkl/yambar)
- lm-sensors (https://github.com/lm-sensors/lm-sensors)
- libsensors (part of lm-sensors)
- C compiler
- make

## Installation

1. Clone this repository:
   ```
   git clone https://github.com/rufusutt/yambar-sensors.git
   cd yambar-sensors
   ```

2. Build the project:
   ```
   make
   ```

3. Install the module:
   ```
   sudo make install
   ```

## Usage

1. Ensure that lm-sensors is properly configured on your system:
   ```
   sudo sensors-detect
   ```

2. Add the yambar-sensors module to your yambar configuration file
    (usually `~/.config/yambar/config.yml`):
    ```yaml
    - script:
        path: /usr/local/bin/yambar-sensors
        args: ["coretemp-isa-0000", "15"]
        content:
          - string: {text: "CPU: "}
          - string: {text: "{temp1_input:.0}°C"}
    ```
    In this example the chip name is `coretemp-isa-1000` and the CPU temperature
    input is `temp1_input`.

3. Restart yambar to apply the changes:
   ```
   killall yambar
   yambar &
   ```

## Configuration Options

Configuration is provided as arguments.

- `chip`: The name of the sensor chip to monitor (required)
- `interval`: Polling interval in seconds (required)

## Available Sensor Data

The module provides access to all readable sensors from the specified chip.
Use the sensor names as variables in your yambar configuration. Common sensor
names include:

- `temp1_input`, `temp2_input`, etc.: Temperature sensors
- `fan1_input`, `fan2_input`, etc.: Fan speed sensors
- `in0_input`, `in1_input`, etc.: Voltage sensors

Run the `sensors -u` command to see available sensors for your system.

## Troubleshooting

If you encounter issues:

1. Ensure lm-sensors is correctly configured, run `sensors`
2. Check that the specified chip name is correct
3. Verify that yambar is running with the correct configuration file
4. Check yambar stderr for any error messages
