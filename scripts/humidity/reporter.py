# Complete Project Details: https://RandomNerdTutorials.com/raspberry-pi-dht11-dht22-python/
# Based on Adafruit_CircuitPython_DHT Library Example

import time
import board
import adafruit_dht

# Sensor data pin is connected to GPIO17
sensor = adafruit_dht.DHT22(board.D17)
# Uncomment for DHT11
#sensor = adafruit_dht.DHT11(board.D17)


try:
    # Print the values to the serial port
    temperature_c = sensor.temperature
    humidity = sensor.humidity
    print(
            "temp={:.1f},humidity={}% ".format(
                temperature_c, humidity
            )
        )
except RuntimeError as error:
    # Errors happen fairly often, DHT's are hard to read, just keep going
    print(error.args[0])
except Exception as error:
    raise error


sensor.exit()