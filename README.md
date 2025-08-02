# BTMS
## Battery Heat Source UDF

The `battery_heat_source.c` file provides a simple example of a Fluent UDF
that applies a constant volumetric heat generation rate for a battery pack.
Adjust the macros `BATTERY_HEAT_POWER` and `BATTERY_VOLUME` as required.
Compile the UDF with Fluent and hook it as a cell zone source term.
