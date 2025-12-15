# MaxxFan BLE Client Implementation Specification

## Overview

This document describes how to implement a BLE (Bluetooth Low Energy) client to control a MaxxFan ventilation fan via the MaxxFan Controller device.

## BLE Service and Characteristics

### Service UUID
```
4fafc201-1fb5-459e-8fcc-c5c9c331914b
```

### Characteristics

#### 1. Command Characteristic (Write-Only)
- **UUID**: `beb5483e-36e1-4688-b7f5-ea07361b26a8`
- **Properties**: Write
- **Purpose**: Send fan control commands
- **Format**: JSON string (UTF-8)

#### 2. Status Characteristic (Read + Notify)
- **UUID**: `cba1d466-344c-4be3-ab3f-1890d5c0c0c0`
- **Properties**: Read, Notify
- **Purpose**: Read current fan state and receive state change notifications
- **Format**: JSON string (UTF-8)

## Device Name

The BLE device advertises as: **"MaxxFan Controller"**

## JSON Command Format

Commands are sent as JSON strings to the Command Characteristic. All fields are optional - only include fields you want to change.

### Command Structure

```json
{
  "mode": "auto" | "manual" | "off",
  "temp": <integer> | "temperature": <integer>,
  "speed": <integer>,
  "lidOpen": <boolean>,
  "airIn": <boolean>,
  "off": <boolean>
}
```

### Field Descriptions

| Field | Type | Values | Description |
|-------|------|--------|-------------|
| `mode` | string | `"auto"`, `"manual"`, `"automatic"`, `"off"` | Operating mode. `"off"` overrides other settings. |
| `temp` or `temperature` | integer | -2 to 37 | Target temperature in Celsius (for auto mode) |
| `speed` | integer | 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 | Fan speed percentage (for manual mode) |
| `lidOpen` | boolean | `true`, `false` | Whether the lid is open |
| `airIn` | boolean | `true`, `false` | Air direction: `true` = air in, `false` = air out |
| `off` | boolean | `true`, `false` | Turn fan off (overrides mode) |

### Command Examples

#### Turn fan off
```json
{"off": true}
```
or
```json
{"mode": "off"}
```

#### Set to automatic mode at 22°C
```json
{"mode": "auto", "temp": 22}
```

#### Set to manual mode, 70% speed, lid open
```json
{"mode": "manual", "speed": 70, "lidOpen": true}
```

#### Set to manual mode, 50% speed, air in, lid closed
```json
{"mode": "manual", "speed": 50, "airIn": true, "lidOpen": false}
```

#### Change only the speed (keeps current mode)
```json
{"speed": 80}
```

#### Change only the temperature (keeps current mode)
```json
{"temp": 25}
```

## JSON Status Format

The Status Characteristic returns the current fan state as a JSON string:

```json
{
  "mode": "auto" | "manual" | "off",
  "temperature": <integer>,
  "speed": <integer>,
  "lidOpen": <boolean>,
  "airIn": <boolean>,
  "off": <boolean>
}
```

### Status Example

```json
{
  "mode": "auto",
  "temperature": 22,
  "speed": 20,
  "lidOpen": false,
  "airIn": false,
  "off": false
}
```

## Connection Flow

1. **Scan for BLE devices** with name "MaxxFan Controller"
2. **Connect** to the device
3. **Discover services** and find the service with UUID `4fafc201-1fb5-459e-8fcc-c5c9c331914b`
4. **Discover characteristics**:
   - Command: `beb5483e-36e1-4688-b7f5-ea07361b26a8`
   - Status: `cba1d466-344c-4be3-ab3f-1890d5c0c0c0`
5. **Subscribe to notifications** on the Status Characteristic (optional but recommended)
6. **Read initial state** from Status Characteristic (optional)
7. **Send commands** by writing JSON strings to Command Characteristic
8. **Receive state updates** via notifications on Status Characteristic

## Platform-Specific Implementation Notes

### Android (Java/Kotlin)

```kotlin
// Example UUIDs
val SERVICE_UUID = UUID.fromString("4fafc201-1fb5-459e-8fcc-c5c9c331914b")
val COMMAND_UUID = UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26a8")
val STATUS_UUID = UUID.fromString("cba1d466-344c-4be3-ab3f-1890d5c0c0c0")

// Enable notifications
bluetoothGatt.setCharacteristicNotification(statusCharacteristic, true)
val descriptor = statusCharacteristic.getDescriptor(
    UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
)
descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
bluetoothGatt.writeDescriptor(descriptor)

// Send command
val command = """{"mode":"auto","temp":22}"""
commandCharacteristic.value = command.toByteArray(Charsets.UTF_8)
bluetoothGatt.writeCharacteristic(commandCharacteristic)
```

### iOS (Swift)

```swift
// Example UUIDs
let serviceUUID = CBUUID(string: "4fafc201-1fb5-459e-8fcc-c5c9c331914b")
let commandUUID = CBUUID(string: "beb5483e-36e1-4688-b7f5-ea07361b26a8")
let statusUUID = CBUUID(string: "cba1d466-344c-4be3-ab3f-1890d5c0c0c0")

// Enable notifications
peripheral.setNotifyValue(true, for: statusCharacteristic)

// Send command
let command = "{\"mode\":\"auto\",\"temp\":22}"
if let data = command.data(using: .utf8) {
    peripheral.writeValue(data, for: commandCharacteristic, type: .withResponse)
}
```

### Python (using bleak library)

```python
import asyncio
from bleak import BleakClient
import json

SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
COMMAND_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8"
STATUS_UUID = "cba1d466-344c-4be3-ab3f-1890d5c0c0c0"

async def send_command(address, command_dict):
    async with BleakClient(address) as client:
        # Enable notifications
        await client.start_notify(STATUS_UUID, notification_handler)
        
        # Send command
        command_json = json.dumps(command_dict)
        await client.write_gatt_char(COMMAND_UUID, command_json.encode('utf-8'))
        
        await asyncio.sleep(1)  # Wait for response

def notification_handler(sender, data):
    status = json.loads(data.decode('utf-8'))
    print(f"Status update: {status}")

# Usage
asyncio.run(send_command("XX:XX:XX:XX:XX:XX", {"mode": "auto", "temp": 22}))
```

## Error Handling

### Invalid JSON
If the command JSON is malformed, the device will ignore it and not send an IR signal. The client should validate JSON before sending.

### Invalid Values
- Temperature values outside -2 to 37 will be ignored
- Speed values not in [10, 20, 30, 40, 50, 60, 70, 80, 90, 100] will be ignored
- The device will use default values for invalid inputs

### Connection Issues
- If the device disconnects, wait a moment and reconnect
- The device will restart advertising after a client disconnects
- Handle connection timeouts gracefully

## Best Practices

1. **Always subscribe to notifications** to receive state updates
2. **Read initial state** after connecting to sync UI with actual state
3. **Validate JSON** before sending commands
4. **Handle disconnections** gracefully and provide reconnection logic
5. **Use appropriate timeouts** for BLE operations (typically 5-10 seconds)
6. **Parse status updates** to keep UI in sync with device state
7. **Send complete commands** when changing modes to ensure all parameters are set correctly

## State Management

The device maintains internal state that is updated when:
- A BLE command is received and processed
- An IR command is received (if IR receiver is enabled)

The Status Characteristic always reflects the last known state. When you send a command, the device will:
1. Parse the command
2. Update internal state
3. Send IR signal to the fan
4. Notify all connected clients of the state change

## Testing

### Test Commands

1. **Basic off/on**:
   ```json
   {"off": true}
   {"off": false, "mode": "manual", "speed": 50}
   ```

2. **Mode switching**:
   ```json
   {"mode": "auto", "temp": 20}
   {"mode": "manual", "speed": 70}
   {"mode": "off"}
   ```

3. **Speed changes**:
   ```json
   {"speed": 30}
   {"speed": 50}
   {"speed": 100}
   ```

4. **Temperature changes**:
   ```json
   {"temp": 18}
   {"temp": 25}
   {"temp": 30}
   ```

5. **Lid and air direction**:
   ```json
   {"lidOpen": true}
   {"airIn": true}
   ```

## Troubleshooting

### Device not found
- Ensure the ESP32C3 is powered on and BLE is initialized
- Check that the device name matches "MaxxFan Controller"
- Try scanning with a generic BLE scanner app first

### Commands not working
- Verify you're writing to the correct characteristic UUID
- Check that JSON is valid and properly encoded (UTF-8)
- Monitor Serial output on the device for error messages

### Notifications not received
- Ensure notifications are enabled on the Status Characteristic
- Check that the descriptor is set correctly (0x2902)
- Verify the notification callback is properly registered

### State out of sync
- Read the Status Characteristic after connecting
- Subscribe to notifications to receive updates
- The device state reflects the last command sent, not the actual fan state (fan doesn't report back via IR)

## Version History

- **v1.0** (2024): Initial specification
  - Single JSON command characteristic
  - Status characteristic with read/notify
  - Support for all fan control parameters


