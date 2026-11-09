# Library metadata retry guard

When metadata reading fails, the Retry metadata action is disabled for a short
cooldown after activation. The request is still sent immediately, but repeated
clicks cannot enqueue duplicate inspections while the bridge is beginning its
recovery attempt. The button retains focus and its existing announcement, then
becomes available again after 750 ms for a deliberate second attempt.

This guard only affects metadata inspection requests; it does not modify source
audio or analysis results.
