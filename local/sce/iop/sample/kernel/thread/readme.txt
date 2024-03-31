[SCEI CONFIDENTIAL DOCUMENT]
PlayStation 2 Programmer Tool Runtime Library Release 2.4
   Copyright (C) 1999 by Sony Computer Entertainment Inc.
                                     All Rights Reserved

Sample program showing examples of multi-threading

<Description>

This program provides basic examples of how to create and start threads, change the priority of threads, and perform synchronization between threads.

<Files>

	createth.c	shows how to create and start 
			threads
	sleepth.c	shows use of SleepThread()/
			WakeupThread()
	eventth.c	shows use of SetEventFlag()/
			WaitEventFlag()
	startthargs.c	shows how to start a thread

<Execution>

	% make		: compilation


Running createth
----------------

Execute the following commands in separate windows:

		% dsicons 

		% dsreset 0 2
		% dsistart createth.irx

The activation status of six threads will be displayed in the window in which dsicons was run.

Execution can also be performed using the following method:

		% dsidb
		> reset 0 2 ; mstart createth.irx

Running sleepth
---------------

First, prepare two windows and then execute the following in each:

	% dsicons 0
	and
	& dsicons 1-9 

Then execute the following in a third window:

	% dsreset 0 2
	% dsistart sleepth.irx

The window in which dsicons 0 is running will display a '0..5, or a..f or A..F>' prompt.

A command input thread that displays the prompt and waits for input will be running along with six other threads.

Although the thread which displayed the prompt and then enters wait mode has a higher priority than the other six threads, since it enters a WAIT state while waiting for key input, the other six threads each receive execution authority and display start messages.
	 
If a number between 0 and 5 is entered and the RETURN key is pressed, the corresponding thread will be woken up (WakeupThread).
	
If a letter between a and f is entered and the RETURN key is pressed, the corresponding #0 to #5 thread is woken up twice (WakeupThread).

If a letter between A and F is entered and the RETURN key is pressed, the corresponding 0 to 5th thread is woken up four times (WakeupThread).
	
The code can also be executed using the following method:

	% dsidb
	> reset 0 2 ; mstart sleepth.irx


If the following run are run in each window, the individual thread's status can be displayed for every TTY which was opened by each thread. 

	% dsicons 1
	% dsicons 2
	% dsicons 3
	% dsicons 4
	% dsicons 5


Running eventth
---------------

First, prepare two windows and then execute the following in each:

	% dsicons 0
	and
	& dsicons 1-9 

Then execute the following in a third window:

	% dsreset 0 2
	% dsistart sleepth.irx


The window in which dsicons 0 is running will display a '0..5,a > ' prompt.

A command input thread that displays the prompt and waits for input will be running along with six other threads.

Although the thread which displayed the prompt and then enters wait mode has a higher priority than the other six threads, since it enters a WAIT state while waiting for key input, the other six threads each receive execution authority and display start messages.

If a number between 0 and 5 is entered and the RETURN key is pressed, the corresponding bit in the event flag will be set.

If 'a' is entered at the prompt and the RETURN key is pressed, all bits in the event flag will be set.

The code can also be executed using the following method:

		% dsidb
		> reset 0 2 ; mstart eventth.irx

If the following run are run in each window, the individual thread's status can be displayed for every TTY which was opened by each thread.
 
	% dsicons 1
	% dsicons 2
	% dsicons 3
	% dsicons 4
	% dsicons 5

Running startthargs
-------------------

Execute the following commands in separate windows:
		% dsicons 

		% dsreset 0 2
		% dsistart startthargs.irx

The starting parameters of the threads will be displayed in the dsicons window.

The code can also be executed using the following method:

		% dsidb
		> reset 0 2 ; mstart startthargs.irx
