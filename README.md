# Pithon

Personal C daemon server to run, initialize, change GPIO pins on Raspberry Pi.

More of a learning project than anything.

Uses Wiringpi.

Compiling:

- mkdir build
- cd build
- cmake -DCMAKE_INSTALL_PREFIX=/usr ..
- make
- sudo make install
- sudo systemctl enable pithond

To do:

- integrate i2c monitoring/logging (reading sensor data and saving to CSV)
- add interupt processing (for an alarm)
