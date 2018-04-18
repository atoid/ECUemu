# ECUemu

## Honda motorcycle ECU emulator / reader

This Linux  application can be used to access Honda motorcycle ECU or emulate ECU responses.
You need interface cable (see my DashBle repo) and preferably FTDI USB-serial adapter to communicate
with the ECU.

Logs folder contain some real data from a running CB500F bike. There is also honda.html 'application'
that can be locally opened in a web browser. Log files can be opened and Javascript parses
the log and shows the data in html tables.

This can be used to visually inspect ECU data table values and maybe decode some of the still
unknown fields.
