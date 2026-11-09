# Library folder transitions

Choosing a new folder is an explicit Library context transition. The previous
source selection, row highlight, search filter, and recursive-catalog state are
cleared before the new folder is browsed, preventing metadata or analysis from
being attributed to an audio file in another folder. Focus returns to the folder
contents so keyboard users can continue immediately. Cancelling the chooser keeps
the current folder and restores focus to the chooser action.

This transition never edits source audio or playback state; it only resets the
Library browsing and selected-source context.
