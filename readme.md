# AMOS client
This is the repository of the AMOS client, a GUI application
that manages a single remote AMOS station.

### 0.8.1.6
Final release of the 0.8. branch, the last to support Qt5 and thus Windows 7.
Further updates are discouraged.

## 1.0
Finally reached the required level of operational readiness. Migrated to Qt6.

### 1.4.0
Added the ability to generate test sightings. Probably will be moved to a separate tab or behind a wall.

### 1.4.1
Removed the possibility of deleted sightings when the server detects a duplicate,
instead they are moved to a separate directory `quarantine`.

This is a response to an incident when the server reported duplicates on an
unrelated database integrity error.

### 1.6.1
Generalized sighting handling so that both UFO and Kvant output is understood, and closed
several ways in which a sighting could be reported as delivered while carrying nothing.

**One sighting per metadata file.** Sightings used to be grouped by the first sixteen characters
of a file name, which is a whole UFO name minus the station but cuts Kvant's event counter off.
Two Kvant events recorded in the same second were collected into one sighting, whose parts then
went up under the same form field names, and the server kept only whichever arrived last.

**Sightings must settle before they are sent.** UFO writes the metadata, the composite, the
thumbnail, the bitmaps and the video in no fixed order, and a two-second scan lands in the middle
of that often enough. A sighting is only built once nothing under its prefix has been written to
for ten seconds; a sighting that has never been sent is also refreshed from disk, in case a part
arrived late.

**Suffixes are compared case-insensitively, and multipart field names are lowercased.** The file
system is case-insensitive and UFO is not consistent about the case it writes. A `.XML` capture
used to be found by the scanner and then uploaded with neither its metadata nor its composite —
accepted by the server, marked stored, and moved to permanent storage, with nothing in it.

**Only the composite image is sent, never the thumbnail.** UFO writes both `<name>P.jpg` and
`<name>T.jpg`, and since every part is named after its suffix, sending both left the server
keeping whichever of the two happened to arrive last. The composite is recognised by the metadata
file beside it: Kvant names it exactly as its metadata, UFO appends a `P`.

**Server responses are dispatched by HTTP status code** rather than by Qt's re-encoding of it —
see below. Along with it, sighting replies are now released after handling; previously each upload
leaked its whole payload, once per attempt.

Also in this release: the dome no longer deletes its serial port manager twice on shutdown, which
crashed on exit; the heartbeat timer logs station state again, so `state.log` is written; and the
temperature displays share a single colour formatter whose cold end is clamped instead of running
past hue 360 and returning an invalid colour below about -45 °C.

## Talking to the server

Heartbeats are posted as JSON to `/station/<id>/heartbeat/`, sightings as multipart to
`/station/<id>/sighting/`. The multipart carries a `meta` part (`spectral`, `timestamp`, `avi_size`,
the last being null when there is no video) plus the metadata file and the composite image. **Field
names are lower case** — `meta`, `xml`, `yaml`, `jpg` — and the server looks them up exactly; get
the case wrong and it answers `201` with `filename: null`, meaning a row was created with nothing
attached.

The HTTP status code is the whole contract, and the client acts on it as follows:

| status        | action                                                        |
|---------------|---------------------------------------------------------------|
| 200, 201      | accepted, moved to primary storage                            |
| 400           | retried indefinitely                                          |
| 409, 422      | quarantined: retained locally, never sent again                |
| anything else | retried indefinitely                                          |
| no reply      | retried indefinitely                                          |

A 400 is retried rather than quarantined because it is usually not this client's fault. The `meta`
part is generated mechanically and cannot be malformed, whereas the server answers 400 for every
request from every station at once when the posted host is missing from its `ALLOWED_HOSTS`, and a
proxy answers 400 for a truncated request body. Quarantining on 400 would move a whole network's
good data out of the send path over a server-side misconfiguration.

Conversely, **to make a station stop resending something the server must answer 422**, not 400.

Dispatching on the status directly matters: Qt's `QNetworkReply::NetworkError` collapses distinct
statuses onto the same value — 413 and 422 both become `UnknownContentError` — so a sighting merely
too large for the proxy used to be quarantined as if the server had refused it.
