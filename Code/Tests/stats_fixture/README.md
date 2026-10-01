# Local stats fixture

This test service supplies chosen statistics to the rebuilt client's BFHQ views.
It listens on `127.0.0.1`, writes no files, and makes no outbound requests.

## Run

From the repository root:

```powershell
python Code/Tests/stats_fixture/bf2_stats_fixture.py --score 20 --weapon 0 --kills 5 --deaths 6
```

Use `--help` for all options. `--selftest` prints sample player, awards, and
leaderboard responses and exits. Weapon 0 is the assault rifle, kit 1 is Assault,
and map 0 is Kubra Dam.

Set this environment variable for the rebuilt client, retaining the trailing
slash:

```text
BF2_STATS_URL=http://127.0.0.1:18123/ASP/
```

The client logs the override once on stderr. Only persistence requests use this
service; login, server browsing, and joining retain their normal paths. Log in
through the normal client flow. Player statistics are cached for an hour, so
restart the client after changing fixture values.

## Responses

| Endpoint | Fixture data |
| --- | --- |
| `getplayerinfo.aspx` | The three player-statistics queries used by the client |
| `getawardsinfo.aspx?pid=N` | Nine sample awards using the stock catalogue IDs; `--no-awards` returns none |
| `getleaderboard.aspx` | Global pages and the requested player's row, including score, team, kit, vehicle, and weapon boards |
| Other endpoints | `E 404` |

Leaderboard defaults are 57 players, the fixture player at position 42, and
player ID 1. Set `--lb-size`, `--lb-position`, and `--lb-pid` to change them.
Use the client's profile ID for `--lb-pid` when testing its highlighted row.

## Format constraints

Player-info responses contain `O`, the `H asof`/`D <time>` pair, a header and
data row, and a trailing `$ n $`. The favourites query reads columns by position,
so preserve their order. Omit unavailable keys instead of emitting empty tab
fields, which the client's tokenizer would collapse.

Awards use the `award level when first` columns. Leaderboards include the
`H size asof`/`D <size> <asof>` pair before their columns and rows. The fixture
sends `deathsby`; the client reads the vehicle/weapon column as `detahsby`, so
that displayed value remains zero.

These responses are test data. They do not validate a ranked-server statistics
pipeline.
