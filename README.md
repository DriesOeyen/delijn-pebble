# De Lijn for Pebble
Dutch-language application for the Pebble smartwatch capable of displaying live departure times for buses and trams operated by the Belgian transportation authority De Lijn.
This application uses the public but undocumented API used by De Lijn's official website.
In other words: the app displays live data straight from De Lijn, including reported delays.

**Note:** this was one of my first experiments with C, so there might be a Y2K-bug or two hiding in the source code. :bug:

## Compatibility
This app was written for the older Pebble SDK 2 and is no longer maintained.
Color and round screens were only introduced in Pebble SDK 3.
Hence, Pebbles with a color screen will display the app in black and white.
Furthermore, Pebble Time Round is not compatible.

The app should run on:

- Pebble Classic
- Pebble Steel
- Pebble Time
- Pebble Time Steel
- Pebble 2

## Installing
Pick your poison:

- Download from the Pebble appstore: https://apps.getpebble.com/en_US/application/54736f68c13ebfe8f5000091
- Get a compiled PBW on the Releases tab: https://github.com/DriesOeyen/delijn-pebble/releases
- Build it yourself: `pebble build` with Pebble SDK version 2
