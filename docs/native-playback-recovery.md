# Native playback recovery

When native output startup fails, the Library's **Retry native** action retries
the bridge's saved format and device selection. The result is announced through
the recovery status channel so keyboard and assistive-technology users receive
the same success or failure feedback as pointer users.
