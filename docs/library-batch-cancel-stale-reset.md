# Library batch cancel stale-result reset

Cancelling an admitted analysis batch now invalidates previous visualization
evidence immediately after cancellation dispatch. The source selection and batch
recovery controls remain available while a replacement accepted snapshot is
awaited, preventing cancelled work from appearing current.
