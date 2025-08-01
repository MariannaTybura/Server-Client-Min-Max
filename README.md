# Server-Client System for Finding Min and Max in an Array (C11)

## Project Description

This project implements a simple server-client system written in C (C11 standard).  
The server provides a service that calculates the minimum and maximum values in an array of integers (int32_t)  
sent by the client using shared memory.

## Features

- The server handles one client at a time.  
- Each client request is processed in a separate thread on the server side.  
- The client reads an array from a file provided as a command-line argument and sends it to the server.  
- The server returns the minimum and maximum values from the sent array to the client.  
- The client waits if the server is busy.  
- The server supports interactive commands:  
  - `quit` – terminates the server after completing all ongoing requests,  
  - `stat` – displays the total number of requests and the average size of processed arrays,  
  - `reset` – resets the statistics.  
- Waiting for commands does not block client handling.  
- All created files and resources are cleaned up when the programs exit.  
- The client detects if the server is not running and displays an appropriate message.
- Communication between the server and client is done exclusively via shared memory.
- Arrays can be of any size.
- Server shutdown properly finishes all active tasks.

## Build and Run

1. Compile the programs:  
   ```bash
   make all
2. Start the server:
   ```bash
   ./server
3. Run the client, providing the input file:
   ```bash
   ./client filein.txt

