# Security and privacy

Cyberputer is a costume display and BLE exploration tool, not a validated
anti-stalking or personal-safety system. Watchlist matches are heuristic,
spoofable protocol/name matches, not evidence of wrongdoing.

## Data and radio behavior

Visualizations keep a bounded in-memory observation table. BLE advertisements
can contain identifying names. The inherited menu utilities can log device
identifiers, GPS coordinates and connection data to microSD.
Do not publish real scan logs or screenshots containing private names/locations.

Visualization active scanning transmits scan requests but does not pair or
connect. Menu connection and sound tools can actively interact with devices;
use them only where authorized.

The inherited Wi-Fi access point has a public default password
(`ghostble123!`). Configure credentials before enabling network utilities and
do not expose the dashboard to untrusted networks. Default credentials are
not a security boundary.

## Reporting issues

Include the visible firmware version, hardware model, reproduction steps and
sanitized diagnostics. Use synthetic names and remove addresses, GPS coordinates,
credentials and tokens from reports. Do not post exploitable details or private
data in a public issue; use the hosting platform's private vulnerability-reporting
channel when enabled, or arrange a private channel with the maintainer.
