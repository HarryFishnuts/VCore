## VCore
The *Core DLL* for VEngine. \
Well, that's what I've planned for it to be. \
I haven't actually started writing VEngine just yet. \
I'll take it step by step!

## What's in it?
VCore has 4 main components:
- Buffering
- File I/O
- Locking
- Logging

## Buffering
VCore has fixed sized buffers which each have an associated synchronization object. \
The buffers can be 'locked' and 'unlocked' as means of synchronization. \
Buffers should be treated more like an opaque structure that can give out a fixed number of fixed pointers as opposed to an array. \
Every element in the buffer is marked as either 'used' or 'unused'. \
It's up to the user to ensure that the contents of the buffer are handled safely.

## File I/O
The file I/O provided is very simple and acts as somewhat of a wrapper around WIN32's file handling functions. \
Lots of generally untouched parameters are hidden away, and all error handling is quickly done behind the scenes.

## Locking
VCore's locking system is a straightforward means of thread synchronization. \
'Locks' can be created, and then 'locked' and 'unlocked' as means of synchronization. \
'Locks' are expected to be deleted at the end of their lifetime.

## Logging
The logging system aims to allow the user to view a snapshot of their program's execution after the process has terminated so that debugging can be made easier. \
The logging system is to be used sparingly and only for significant events.

There are 3 entry levels:
- Info
- Warning
- Error

Info should record a significant event which is key to program execution.\
Warnings should record a significant event which may indicate a bug or a soon-to-be process crash. \
Errors should record an event which will immenently crash the program. All recent logs are dumped to the disk when an error is recorded. It is not expected for many errors to occour in sequence without the process crashing.
