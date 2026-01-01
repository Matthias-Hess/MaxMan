import subprocess
import os

Import("env")

def get_firmware_version():
    try:
        # Fragt git nach dem aktuellen Tag (z.B. "v1.0.2")
        # "describe --tags" ist schlau: Wenn du 2 Commits NACH dem Tag bist,
        # liefert es etwas wie "v1.0.2-2-g3a1b" -> Perfekt für Dev-Builds!
        ret = subprocess.check_output(
            ["git", "describe", "--tags", "--always", "--dirty"], 
            stderr=subprocess.STDOUT
        ).strip().decode("utf-8")
        return ret
    except:
        return "v0.0.0-unknown"

VERSION = get_firmware_version()

print(f"Building Firmware Version: {VERSION}")

# Hier wird das Makro für C++ definiert
# Das entspricht: #define APP_VERSION "v1.0.2"
env.Append(CPPDEFINES=[
    ("APP_VERSION", f'\\"{VERSION}\\"')
])