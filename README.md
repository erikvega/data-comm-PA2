# go-back-n

barbones go-back-n starter. the program serializes packets on client side, sends to server, and the server deserializes them and writes the data to a file given through the command line 

## things that are missing from this 
- checksum 
- sliding window
- timer
- error handling on client side 

## running the program

 - use the makefile to compile all necessary files
 - start the server up first by using 
```
./server 12199 output.txt
```
- start the client up 
```
./client localhost 12199 input.txt
```
