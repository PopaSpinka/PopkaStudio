# The player's music

`music.bin` — the tracks for the Music app, packed for the board's "ffat" flash partition (at 0x910000) by
HaloWatch's `tools/halo_music.py`: 20-second excerpts of four free recordings from [Musopen](https://musopen.org),
taken from Wikimedia Commons (each license checked on the file's page on 30 October 2026). Load writes it once, when
the project has the Music app and the board has no tracks yet.

| Piece | Performer | License |
| --- | --- | --- |
| [Erik Satie — Gymnopédie No. 1](https://commons.wikimedia.org/wiki/File:Erik_Satie_-_gymnopedies_-_la_1_ere._lent_et_douloureux.ogg) | Robin Alciatore | public domain |
| [J. S. Bach — Goldberg Variations, Aria](https://commons.wikimedia.org/wiki/File:Bach,_Goldberg_Variations,_Aria_(Musopen_version).ogg) | Shelley Katz | CC0 1.0 |
| [L. van Beethoven — Sonata No. 8 "Pathétique", II. Adagio cantabile](https://commons.wikimedia.org/wiki/File:Beethoven,_Sonata_No._8_in_C_Minor_Pathetique,_Op._13_-_II._Adagio_cantabile.ogg) | Paul Pitman | CC0 1.0 |
| [F. Chopin — Nocturne Op. 15 No. 1](https://commons.wikimedia.org/wiki/File:Chopin_-_Nocturne_Op._15_no._1_in_F_major.ogg) | Luke Faulkner | CC0 1.0 |
