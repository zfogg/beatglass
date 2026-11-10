# Library storage lifetime

`StorageOwner` is a worker-owned SQLite boundary. Open, use, close, and destroy
it on the same database worker. Before calling `close()`, cancel or stop every
job that can access the cache and join those jobs. `close()` only releases the
cache; it does not wait for executor work or marshal calls between threads.

After close (or a failed open), cache operations are inert: reads return empty
results, admissions are empty, and publication reports an error. Re-open only
after the previous worker has quiesced. This preserves ownership and prevents
use-after-close races while allowing a failed reopen to leave the owner safely
closed.
