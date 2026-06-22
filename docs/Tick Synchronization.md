## The Problem

On one side, there's the main thread of the Blam game engine, the one that executes all the game's logic, and the **logic-ticks**. On the other side, there's the AI thread of Artemis, which consumes the generated data from those logic-ticks when it reads the *object table*, generating the Artemis logic-tick.

The problem is, that the AI thread and the Game's thread run asynchronously. This means it's possible that the AI thread could be reading the *object table* while the game is writing on it, generating unstable, corrupted and unreliable data for Artemis logic-tick. As the project needed to have reliable logic-ticks, I needed to ensure that the readings occur always at the correct time, when Blam just finished the write of its logic-tick.
## Mental Model: Blam-Artemis tick synchronization

So, if Artemis is able to know when exactly Blam ends the writing of its logic-tick, then it knows exactly when it has to start reading. Then, the most reliable way to resolve this synchronization problem, is to find a Blam function that defines when a logic-tick is going to be executed, and  that function is FUN_180057434 (SimulationTicks), who receives as parameter, the amount of ticks that has to be executed at any given moment, and if that amount is higher than zero, then it calls FUN_1800575AC (ExecuteGameFrame), who is the real processor of the logic-tick of Halo Reach.

Therefore, once we are able to hook this function, we just need to send a signal to execute the Artemis logic-tick, **after** the logic-ticks were executed. In this way, we would be able to read the data as soon as these logic-ticks finished writing. While this is happening, the AI thread will be *waiting* to receive the signal, to proceed and execute one Artemis logic-tick.

But, since we are in a multi-threading context, sending signals between threads is not that simple, and a flag is not enough in this case, because we need to know if we weren't able to process a Blam logic-tick, for example, if Artemis logic-tick is taking too much time to be processed. That's why we need a tick counter.

Also, we have to consider the context where we are working on, a game engine with its own life-cycle. For instance, what happens if the AI thread is waiting for a Blam logic-tick when the game engine has started closing itself? Artemis needs a way to react to the current state of Blam, and here is where the hook to FUN_18000C8F8 (DestroySubsystems) plays its role. This function is called by Blam whenever the game engine has started closing itself. Artemis uses this hook to warn its subsystems to close themselves, **before** Blam ends its closing.
## Design Decisions

Because of the multi-threading context, the use of atomic variables was practically a necessity. But, I could have used a ```std::atomic<bool>``` as a signal to determine if Artemis had to execute a logic-tick, but, as I mentioned earlier, a boolean wouldn't allow me to quantify if I **lost** Blam logic-ticks. For this reason, I decided to use a ```std::atomic<uint64_t>```, not only to save the dropped ticks, but to be able to know the executed Artemis logic-ticks, and to be able to allow the lambda to  compare the tick counter value with the last seen one.

 Atomic variables protected any thread from obtaining half-finished data of the variable, but they didn't protect by default from the possibility that the increased value of the tick were propagated **before** the logic-tick data of Blam, generating the read of the **previous** logic-tick data by the AI thread. For this reason, the selection of the correct ```memory_order``` was so important.

```SignalTick()```, called from the main thread of Blam, increments the ticks counter using ```std::memory_order_release```, that ensures the modified structures by the Blam logic-tick were written and visible for any other thread before the value of the tick were incremented. 

```WaitForTick()```, called from Artemis AI thread, sleeps the thread while it waits the ticks counter to be incremented. Whenever the thread is awakened by ```notify_one()``` or *spurious wakeups* , it gets the ticks counter by using ```std::memory_order_acquire```, that ensures what was written before the previous ```std::memory_order_release``` it is already visible from the AI thread.

This ensures Artemis from reading outdated logic-tick data, but how was able the AI thread to securely wait for the tick counter to be increased in the first place? 

By using a condition variable. Combining it with a mutex was necessary to avoid any *lost wakeup*, since it protects against the case of ```SignalTick()``` sending the ```notify_one()``` while the condition variable has not yet caused the AI thread to go to sleep by using ```wait()```. And the lambda of the CV ensures that, before the AI thread goes to sleep, it always checks if the ticks counter is **different** from the last known one, and if it is, the ```wait()``` is ended, proceeding to execute one Artemis logic-tick.

Lastly, if the subsystem loses any ```notify_one()``` while it is occupied performing the Artemis logic tick, it doesn't really matter, since the ticks counter serves as a source of truth, telling us the exact amount of Blam logic-ticks that were lost during that time. If the dropped logic-ticks are higher than 0, then it means the Artemis logic-tick is too slow and it has to be optimized or simplified.