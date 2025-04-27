# BraveBrowser Documents

> this is a document about brave broswer

---

## Table of contents

- [About Chormium](#aboutchormium)
- [Tools](#tools)
- [Core Concepts](#core-concepts)
- [Relative Info](#relative-info)

---

## [AboutChormium](https://www.chromium.org/developers/design-documents/)
> Brave is based on chormium, so I'm research Chormium frist here

### Multi-process Architecture

- Chormium uses multiple processes to avoid the crash of one process causes the entire browser to crash
    - processes
        - borwser process
            - main process that runs the UI and manages renderer and other processes
        - renderer process
            - handle web content
            - use Blink open-source layout engine for interpreting and laying out HTML
            - [what is Blink?](#blink)
                
    - the communication between renderer process and browser process
        - RenderProcess(This is a global object instead of the process object above)
        - RenderProcessHost
            - maintained by browser process
            - manages browser state and communication for the renderer
        - the above two use [Mojo](#mojo) and [Chromium's legacy IPC system](#chromiums-legacy-ipc-system) to communicate

    - manageing frames and documents
        - Each renderer process has one or more RenderFrame objects, which correspond to frames with documents containing content: 
            - like an website, it will nests several iframes. For each such frame, there will be a RenderFrame instance in the rendering process responsible for processing
        - The corresponding RenderFrameHost in the browser process manages state associated with that document.
        - conclusion: Each document/iframe (frame) in the browser is handled by a RenderFrame in the rendering process. The browser process manages it through the corresponding RenderFrameHost and communicates with it through Mojo or IPC. In order to uniquely identify a frame, you need: which rendering process (RenderProcessHost) + which routing ID

---

### Multi-threded
>Chromium's thread design follows the "Task-Based Parallelism" principle, rather than the traditional [thread lock competition mode](#thread-lock-competition-mode).

- Follow the principle of "Task-Based Parallelism"
    - every thred has an unique MessageLoop, Process tasks in the "Task Queue" to form a "task chain"
        - every thred has it's own MeasseageLoop and TaskQueue
        - If a thread needs to interact with another thread: 
            - PostTask: this method will send the task to task list for a specified thread

- Main thred
    - UI thred in the process 
        - Handling user input (mouse/keyboard), window management, address bar, download UI
---

### Things to note when modifying the code

- Do not perform expensive computation or blocking IO on the main thread
---

### About Thred
- Thred Construction
    - one main thred
        - Browser Therd(browser process)
        - Blink main thred(renderer process)
    - one IO thred
        - usage: recieve IPC/Mojo message
        - Usually does not process business logic, but forwards it to other threads for processing after receiving it.
        - Asynchronous IO Operations
    - few Special-purpose Threads
    - [thred pool](#thred-pool)
- [Lexicon](#thred-lexicon)
    - these are not the type of thred, more like a descriptive label

### About Task
> A task is a base::OnceClosure added to a queue for asynchronous execution.
- [base::OnceClosure](#callback-and-bind)
    - core concept: create function closures by base::Bindonce
        - base::BindOnce will bind a function (such as TaskA or TaskB) to its parameters, generating a task that can be executed later.
            ```cpp
            void TaskA() {}
            void TaskB(int v) {}

            auto task_a = base::BindOnce(&TaskA);  //bind taska
            auto task_b = base::BindOnce(&TaskB, 42);  //bind taskb and 42
            ```
- Task execution method
    - parallel
    - sequenced
    - single threded
    - COM single Threded

- Posting a Parallel task
    - PostTask (direct posting to the thred pool)
    - Post via taskrunner
    - Post sequenced task(not necessarily on the same thred)
        - How to implement a series of synchronous tasks
            - Ⅰ.create an sequenced taskrunner
            - Ⅱ. post tasks on taskrunner in order 
        - By chormium's rules, Sequence tasks should be given priority
    - How to run multiple tasks on same thred
        ```cpp
        base::SingleThreadTaskRunner
        ```
    - post task to main thred or IO therd
        ```cpp
        content/public/browser/browser_thread.h
        ```
        - content::GetUIThreadTaskRunner({})
        - content::GetIOThreadTaskRunner({})

- Important guidelines you can follow
    - just post 

- Efficient parallel task scheduling method - base::PostJob
    - woker task 
        - The threads in the thread pool run this WorkerTask together
        - The program loops through small work items until it is finished or needs to exit.
    ```cpp
    void WorkerTask(base::JobDelegate* job_delegate) {
        while (!job_delegate->ShouldYield()) { // 一直做，直到该让路
            auto work_item = TakeWorkItem();     // 拿一个小任务
            if (!work_item)
            return;                           // 没任务了就退出
            ProcessWork(work_item);              // 处理任务
        }
    }
    ```

- general use API
    > These are the APIS to contact with SequenceManager
    - base::RunLoop
        - start sequence manager

    - base::Thread/SequencedTaskRunner::CurrentDefaultHandle
        - re post task from current thred/sequence
    
    - base::SequenceLocalStorageSlot
        - Attach some local variables to the current sequence (each sequence has its own small storage).
    
    - base::CurrentThread
        - For example, use base::CurrentThread::Get()->GetTaskRunner() to get the task queue of this thread.
    
---
### About Components

---
### [Inter-process Communication(IPC)](https://www.chromium.org/developers/design-documents/inter-process-communication/)

#### IPC in the browser
- I/O thred
    > I/O threads mainly exist in the browser process
    - this is a thred that dedicated to handling network requests and other input/output tasks
    - If you are checking the url request like me, you may pay more attention to the IO therd

- ChannelProxy
    - master that send meesage from renderer to browser process

- ResourceDispatcherHost(资源调度主机)
    - components that handles resoure request
    - Call the corresponding network protocol for processing according to the content of the request (such as HTTP request)


#### IPC in the renderer
- main rederer thred 
    - Manages messages from and back to the browser process

- renderer thred
    - Also called WebKit thread, it is mainly responsible for actual page rendering, JavaScript execution, and UI updates.

- IPC
    - In the renderer process, messages are passed from the browser process to the WebKit renderer thread via the main renderer thread, and vice versa.


#### Multi-process Reasource loading
> All betwork communications are handled by Browser process, so if you only focus on network requests of ominibox, you don't need to change the IPC-related code

- We can divide it into three parts
    - Blink: render engine
    - Renderer: every render process,which always be an page, it includes an Blink object
    - Browser: Manage all render process and handle all the network requests

#### Messages
- Type
    - routed
    - control


---

### Writing Tests
    - TEST_F marco

---

### UI Thred and IO Thred
> super busy threds 
#### UI (Main Thred)
- Handles all UI related stuff

- Schedule various high-level logic inside the browser

- Manage all the objects which is relative with UI

- Runs most of the core logic of the Browser process
#### IO (Main Thred)
- Handle IPC meassage
    - Mojo communication
- Network Request

- Handles disk IO (small amount)
    - read file
- Interact with underlying platform services
    - For example, DNS resolution and proxy settings change monitoring
---

## Core Concepts

### Task 
> unit of work to be processed

- Includes: function pointers and relative state data
- Impl class: 
    - base::onceCallback
        - this is One-time tasks
    - base::RepeatingCallback
        - Repeatable tasks

### Task Queue
> A first-in-first-out (FIFO) task queue

- Scheduling strategy
    - priority
        ```cpp
        base::TaskPriority
        ```
    - delay
        ```cpp
        base::DelayedTask
        ```

### Physical thread
> A Physical Thread is a real, operating-system-level thread (e.g. pthread on Linux/macOS, or CreateThread() on Windows)

- The actual thread resources allocated by the operating system

- for example: UI thred, my operation system allocate thred 1 to it

- UI thred's Physical thred is thred 1. 

- At this point, this thread is exclusive to the UI thread

### base::Thread
> A thread wrapper class that represents a physical thread and is used to execute background tasks or long-running tasks.

- built-in messageloop
    - Each base::Thread instance maintains a MessageLoop (or SequenceManager) to automatically handle the scheduling of task queues.
- start and stop
    - Start() and Stop()

### Thred pool
> Globally shared set of physical threads

    base/task/thred_pool.h

- Global Singleton
    - Each Chromium process (browser, renderer, etc.) has only one base::ThreadPoolInstance instance. 
- Smart "Task Scheduler"
    - Find an idle thread suitable for executing this task to handle it

### Sequence / Virtual Thread
> An execution order model defined by Chromium itself
- Summary
    - Give an order, and then let the free threads help you complete these tasks one by one in order
    - It is a concept within the process and cannot cross processes.

### TaskRunner
> interface: "Throw" the task into the queue for execution
- Required Information
    - what task to do
        - function 
        - callback
    - when to do 
        - how much time usr want to delay
    - how to do
        - which thred will be used
- process limited
    - Only tasks in the current process can be scheduled, not across processes

### Sequenced task runner
> special taskrunner: Tasks must be executed one by one in sequence

### SingleThreadTaskRunner
> as its name: It ensures that all tasks are executed by the same physical thread

### Callback<> and Bind()
    base/functional/bind.h

- Partial application
    - bind a subsets of arguments to produce another function that takes fewer argument
        ```cpp
        int Add(int a, int b);
        auto Add5 = BindOnce(&Add, 5);  // Add5 是一个只需要一个参数（b）的新函数
        Add5.Run(3);  // 相当于调用 Add(5, 3)
        ```
    - Differences between closure
        - In javaScript, python, Lisp: retain a reference to its enclosing environment
        - What is captured is the parameters you explicitly pass in, and the "binding" is completed when Bind is called.

- base::OnceCallback
    - A wrapper for a function that is called only once
        #### OnceCallback<>
        - created by base::BindOnce()
        - move-only type: means it can be moved but not copied

        #### RepeatingCallback<>
        - created by base::BindRepeating()
        - copies cheap
        - Can be implicitly converted to base::OnceCallback<>
    
- [usage](https://chromium.googlesource.com/chromium/src/+/HEAD/docs/callback.md)
    - this is doc about the callback's usage

        
- base::OnceClosure
    - A one-time small task unit with no parameters and delayed execution

---

### Thred lexicon
> Some technical explanations of "thread safety"
- Thread-unsafe
    - the vast majority of types 
    - This property describes whether a class (object) can be used safely in a multi-threaded environment.
    - means this class is not friendly to multi-thred, We have to queue up when we use it

- Thread-affine
    - also a label: means this object can only be used in a specific thread and cannot be accessed by another thread.

- Thread-safe
    - label: Indicates that an object can be accessed by multiple threads simultaneously without error.

- Thread-compatible(线程兼容)
    - Read operations can be performed simultaneously, but write operations require you to lock and control them yourself

- Immutable
    - once you construct the thred, it's immutable

- Sequence-friendly
    - These types or methods are not inherently thread-safe by design, but they support calling from base::SequencedTaskRunner



## Tools

### Blink

    src/third_party/Blink

Responsible for parsing HTML, CSS, and JavaScript, and rendering web page content into a user-visible interface

---

### Mojo
> Mojo doesn't deal with service
> and it's not same with [modular mojo](#modular-mojo)

- Allow modules in different languages ​​such as C++, Java, JavaScript, etc. to communicate with each other (such as the interaction between the browser process and the rendering process)

- Split various functions of Chromium (such as network, GPU, storage) into independent services, communicate through Mojo, and reduce code coupling

#### Mojo's work principles

- Mojo interface
    - Define the inter-process communication interface (similar to the API protocol) through the .mojom file
    - example: interface is used to declare the "protocol" for inter-process communication
        ```mojom
        interface DocumentParser {
            ParseHtml(string html) => (bool success, string error);
        };
        ```
        - input: string Html
        - return: success: if parse success; error 
        - significance: The Mojo toolchain automatically generates client and server code in multiple languages ​​based on the .mojom file.
- client stub: I will compare it to a food delivery platform' App
    ```cpp
    mojo::Remote<RestaurantService> restaurant;
    restaurant->OrderFood("鱼香肉丝", base::BindOnce(...))
    ```
    - It converts the dishes you want to eat (method calls) into orders (Mojo messages), but you don’t care how the orders are delivered to the kitchen.  
    - You don’t need to know: How the order is packaged (serialization) .. Which route the rider takes (message pipeline)

- Server Skeleton：I will conpare it to a restaurant order taking system
    ```cpp
    class RestaurantImpl : public mojom::RestaurantService {
    public:
    void OrderFood(const std::string& food_name,
                    OrderFoodCallback callback) override {
        bool success = CookFood(food_name); 
        std::move(callback).Run(success);
    }
    };
    ```
    - it will Translate the takeaway order (Mojo message) into a dish name that the chef can understand (actual method call) 
    - Breaking down the order: The cashier sees "Order #123: 鱼香肉丝" and tells the kitchen to make this dish
    - Return result: The dish is ready and packed for takeout (serialized result)

- Binding Glue: I will compare it to rider team
    ```cpp
    // bind your APP and restaurant
    mojo::Remote<RestaurantService> restaurant; // APP
    mojo::PendingReceiver<RestaurantService> receiver = 
        restaurant.BindNewPipeAndPassReceiver(); // generate a order

    // restaurant binds the order system
    mojo::MakeSelfOwnedReceiver(
        std::make_unique<RestaurantImpl>(), // cheif
        std::move(receiver) // order sender
    );
    ```
    - Rider team: responsible for delivering orders from the APP to the restaurant and then delivering the food back.
    - Delivery rules: for example, riders must use the safety channel (thread safety), and orders must be placed in an insulated box (serialization)        
- complete process:
    - You order food → The stub (APP) turns "Fish-flavored Shredded Pork" into an order.
    - The rider delivers the order → The binding code sends the order through MessagePipe.
    - The restaurant receives the order → The skeleton (cashier) disassembles the order and asks the kitchen to cook.
    - The rider delivers the food → The skeleton packs the prepared food and the binding code returns the result.
    - You get the food → The stub triggers a callback: "Your fish-flavored shredded pork has been delivered."
- inclusion
    - Mojo defines a set of safe cross-process/cross-language communication rules
    
---

### Chromium's legacy IPC system
> Mojo's predecessor: Phase out

---




###

## Relative Info

### Modular Mojo
this is a AI/High Performance Computing language

- Fully compatible with Python syntax: Existing Python code can be run directly

- C++-level performance: Thousands of times faster than Python through static compilation, zero-cost abstraction, and other features.

- Optimized for AI/ML: Native support for tensor computing, GPU/TPU acceleration, etc.

---

### Thread lock competition mode

- Shared memory + lock synchronization to achieve concurrency control\

- lock: Basic tools for protecting shared resources in multithreaded programming
    - Ensure that only one thread can operate on shared resources at a time