# The JAR in a harness

Munkiki's Castles' own classes, run on a desktop JVM against stub
implementations of the MIDP 1.0 and Nokia APIs they use (the `javax` and
`com` packages here), driven from a script and logging the game's whole
state after every tick, with the obfuscated field names the JAR has. This
is what the port's C is checked against: `make check-munkiki` runs the
scripts in `tests/munkiki/` through both and compares every field
(tools/munkiki_compare.py).

The stubs are the port's own code. They model the phone's fonts with the
metrics of the fonts the port extracts (build/assets/fontmetrics.txt), draw
nothing in 3D (the M3D calls are logged), and keep record stores in
memory. A script's commands: `tick [N]`, `press K`, `release K`, `key K`
(K: 0-9, star, hash, up, down, left, right, fire, soft1, soft2 the C key),
`select N`, `back`, `more`, `choice G N`, `frame FILE`, `dump`, `# ...`.
