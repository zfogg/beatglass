# Library recovery next action

Terminal analysis recovery now reports how many accepted batch items remain
eligible for retry, while preserving the source-audio safety boundary for discard.
This makes the next action concrete without implying that queued or successful
items need recovery.
