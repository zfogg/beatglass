# File analysis cancellation

`replay_file` accepts an optional `std::stop_token` and passes it to the
worker-owned `FileReader`. Cancellation is cooperative: the current complete
analysis state is returned after the reader observes the request, and no
partial decoded block is sent to replay or observers. This lets a Library
worker stop a long file analysis when the user changes selection or shuts
down, without treating cancellation as a decoder error.
