# GameSpy SDK local changes

This tree uses the December 2007 GameSpy SDK; qr2 identifies itself as version
3.04.00 in [its changelog](qr2/changelog.txt). Preserve the upstream notices and
keep integration changes identifiable in the source.

| File and function | Local change | Reason |
| --- | --- | --- |
| `serverbrowsing/sb_serverbrowsing.c`: `ServerBrowserAuxUpdateIP` | Check for an existing server address before starting and appending a new server. | Avoid rejecting the newly appended server as its own duplicate before the synchronous wait. |
| `qr2/qr2.c`: `qr2_init_socketA` | Initialize `denyresp2_ip_callback` to `NULL` with the other callbacks. | The enabled query-parser hook must not call an uninitialized function pointer. |

Both client and dedicated-server integrations can use these shared SDK files.
Changing the SDK revision requires checking the interfaces and behavior used by
those integrations.
