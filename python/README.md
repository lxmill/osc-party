# OSC controlled sensor


# Setup
```pip install python-osc Bluetin_Echo```

# Run
```python3 osc_sensor.py```

# Accepted OSC messages
 . Change host:port/path to send the values to \
  ```/host/path <address> : [host][:port][/path] ```
  
 . Set the wait period between getting sensor values \
   ```/sensor/delay <value> : number```

# Sends the distance values to the defined host:port/path
