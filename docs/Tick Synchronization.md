## The Problem

On one side, there is the main thread of the Blam game engine, which executes all the game logic, the **logic-ticks**. On the other side, there is the AI thread of Artemis, which processes the data of those ticks.

The two threads run asynchronously, so the AI thread must never read the *object table* while the game is writing it: the data would be unstable and corrupted.

## Mental Model: the game thread copies, the AI thread processes

Blam tells us exactly when it finishes writing a logic-tick: the functions that run the simulation (`SimulationTicks` runs `ExecuteGameFrame` once per tick in a loop). Artemis hooks them, and a tick is complete when the next `ExecuteGameFrame` of the same call starts, or when `SimulationTicks` returns.

At that point the **game thread itself** copies the tick into a Snapshot (`CaptureService`, through the `TickProducer` port) and puts it in a bounded FIFO queue (`TickQueue`, 4 ticks, one producer and one consumer). The engine writes nothing between two ticks, so the copy is stable without any handshake with the AI thread. The game goes on at once. The AI thread takes the queued ticks in order and runs the pipeline over the Snapshots; it never reads game memory.

## Every tick, not every frame

When the engine runs several ticks in one frame (a slow frame, or a game speed above 1x), the hooks hand over every tick, not only the last one of the frame.

## Backpressure

If the AI thread falls more than one decision cycle behind, the queue is full and the game thread waits for room, at most 100 ms (`k_ReadTimeout`). Past it the tick is not queued and is counted as lost (telemetry: dropped ticks), so a stuck AI thread slows the game down but never freezes it.

## The life-cycle of the engine

The game thread queues ticks only while the AI thread consumes them (the queue is open) and has loaded the resources of the current map (`LifecycleStore::MarkLoaded`, compared with the teardown epoch). When the engine tears down, the teardown epoch goes up, so nothing more is queued for the old map. The game thread waits only for the tick the AI thread is processing (`BeginTick`/`WaitForTickEnd`), never for the queued ones: the AI thread discards them, and a tick of an old epoch is never processed against the stores of the new map.

## Design Decisions

The queue is a mutex and two condition variables (`Common.State.Type::BoundedQueue`): the producer waits for room with a timeout, the consumer waits for an entry, and closing the queue wakes the producer. No wakeup can be lost because every predicate is checked under the mutex.

The Snapshots come from a small pool filled only by the game thread: the AI thread releases one by dropping its reference, and the game thread takes it again when only the pool holds it (an acquire fence orders the AI thread's last reads before the game thread rewrites it).

Between popping a tick and starting it the AI thread can lose a race with the teardown, so `BeginTick` goes up first and the epoch is checked after; `BeginTick` and `WaitForTickEnd` put a full fence around their store and load, so one of the two always sees the other.
