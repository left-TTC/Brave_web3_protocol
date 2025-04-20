# BraveBrowser Documents

> this is a document about brave broswer

---

## Table of contents

- [About Chormium](#aboutchormium)
- [Code](#tools)

---

## AboutChormium
Brave is based on chormium, so I'm research Chormium frist here

### Multi-process Architecture

- Chormium uses multiple processes to avoid the crash of one process causes the entire browser to crash
    - processes
        - borwser process
            -  main process that runs the UI and manages renderer and other processes
        - renderer process
            -  handle web content
            - use Blink open-source layout engine for interpreting and laying out HTML
            - [what is Blink?](#blink)
                
    - the communication between renderer process and browser process
        - RenderProcess(This is a global object instead of the process object above)
        - RenderProcessHost
            - maintained by browser process
            - manages browser state and communication for the renderer
        - the above two use [Mojo](#mojo) and [Chromium's legacy IPC system](#chromiums-legacy-ipc-system) to communicate

---



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
        - generated cpp function:
            - client stub: 
            ```cpp
            // Automatically generate document_parser.mojom.h
            namespace mojom {
            class DocumentParser {
            public:
            virtual void ParseHtml(
                const std::string& html,
                base::OnceCallback<void(bool success, const std::string& error)> callback) = 0;
            };
            }  // namespace mojom

            //client's code
            mojo::Remote<mojom::DocumentParser> parser;  
            parser->ParseHtml("<html>...</html>", base::BindOnce(...));
            ```
        
- 
    
---

### Chromium's legacy IPC system

---

## Relative Info

### Modular Mojo
this is a AI/High Performance Computing language

- Fully compatible with Python syntax: Existing Python code can be run directly

- C++-level performance: Thousands of times faster than Python through static compilation, zero-cost abstraction, and other features.

- Optimized for AI/ML: Native support for tensor computing, GPU/TPU acceleration, etc.