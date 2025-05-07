Sensor used include: 
1.) Sensirion SEN55-SDN-T, I²C (addr 0x69)
2.) Gravity IIC Ozone Sensor (SEN0321), I²C (addr 0x73)
3.) Gravity: MEMS Gas Sensor (SEN0377), I²C (addr 0x75)

Run I2C_test to see the sensor's addresses 

Microcontroller used: 
1.) Arduino MEGA 2560 

Links for Library: 
1.) https://github.com/Sensirion/arduino-i2c-sen5x
2.) https://wiki.dfrobot.com/Gravity_IIC_Ozone_Sensor_(0-10ppm)%20SKU_SEN0321
3.) https://wiki.dfrobot.com/_SKU_SEN0377_Gravity__MEMS_Gas_Sensor_CO__Alcohol__NO2___NH3___I2C___MiCS_4514

The logic of the code follows:

1.) Read all sensors (ozone, NO₂, PM₂.₅, PM₁₀).

Compute elapsed time since last loop (using millis()).

2.) Update timer:

If value ≥ threshold → add to total_time_above_X, reset total_time_below_X.

Else → add to total_time_below_X.

3.) Green Warning:

If total_time_above_X ≥ threshold_X_timer and no prior danger and not in “safe” → turn green LED on.

4.) Red Warning:

If total_time_above_X ≥ (threshold_X_timer + extra_overtime) and no prior danger and not in “safe”:

turn red LED & buzzer on, set mark_X = true.

5.) Safety Check:

If any LED is on and all total_time_below_X ≥ time_in_safety:

reset all warning timers, set safe = true, turn LEDs & buzzer off.

6.) Re‐exposure (after being “safe” and having triggered danger once):

If value ≥ threshold → accumulate time_reexpose_X.

If time_reexpose_X ≥ danger_again → re‐activate red LED & buzzer.
